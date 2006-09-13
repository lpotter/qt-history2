#include "qttesselator.h"
#include <QPointF>
#include <QVector>
#include <QList>
#include <QVarLengthArray>

#include "utils.h"

#include "qnum.h"
#include "XrenderFake.h"

/*
 * Polygon tesselator - can probably be optimized a bit more
 */

//#define QT_DEBUG_TESSELATOR
#define FloatToXFixed(i) (int)((i) * 65536)
#define IntToXFixed(i) ((i) << 16)

//inline int qrealToXFixed(qreal f)
//{ return f << 8; }

struct QEdge {
    inline QEdge()
    {}
    inline QEdge(const QPointF &pt1,
                 const QPointF &pt2)
    {
        p1.x = XDoubleToFixed(pt1.x());
        p1.y = XDoubleToFixed(pt1.y());
        p2.x = XDoubleToFixed(pt2.x());
        p2.y = XDoubleToFixed(pt2.y());
        m = (pt1.x() - pt2.x()) ? (pt1.y() - pt2.y()) / (pt1.x() - pt2.x()) : 0;
        im = m ? 1/m : 0;
        b = pt1.y() - m * pt1.x();
        vertical = p1.x == p2.x;
        horizontal = p1.y == p2.y;
    }

    inline qreal xAt(const qreal &y) const
    {
        Q_ASSERT(p1.y != p2.y);
        XFixed yf = XDoubleToFixed(y);

        if (yf == p1.y)
            return XFixedToDouble(p1.x);
        else if (yf == p2.y)
            return XFixedToDouble(p2.x);

        return (!vertical) ? (((y - b)*im)) : pf1.x();
    }

    QPointF     pf1, pf2;
    XPointFixed p1, p2;
    qreal m;
    qreal im;
    qreal b;
    qreal intersection;
    signed short winding;
    bool vertical;
    bool horizontal;
};
Q_DECLARE_TYPEINFO(QEdge, Q_PRIMITIVE_TYPE);

struct QVrtx {
    typedef QList<QEdge> Edges;
    XPointFixed coords;
    Edges startingEdges;
    Edges endingEdges;
    Edges intersectingEdges;
};
Q_DECLARE_TYPEINFO(QVrtx, Q_PRIMITIVE_TYPE);

struct QIntersectionPoint {
    qreal x;
    const QEdge *edge;
};
Q_DECLARE_TYPEINFO(QIntersectionPoint, Q_PRIMITIVE_TYPE);


// used by the edge point sort algorithm
static qreal currentY = 0.f;

static inline bool compareEdges(const QEdge *e1, const QEdge *e2)
{
    return e1->p1.y < e2->p1.y;
}

static inline bool isEqual(const XPointFixed &p1, const XPointFixed &p2)
{
    return ((p1.x == p2.x) && (p1.y == p2.y));
}

static inline bool compareIntersections(const QIntersectionPoint &i1, const QIntersectionPoint &i2)
{
    if (qAbs(i1.x - i2.x) > 0.01) { // x != other.x in 99% of the cases
        return i1.x < i2.x;
    } else {
        qreal x1 = !qIsFinite(i1.edge->b) ? XFixedToDouble(i1.edge->p1.x) :
                   (currentY+1.f - i1.edge->b)*i1.edge->m;
        qreal x2 = !qIsFinite(i2.edge->b) ? XFixedToDouble(i2.edge->p1.x) :
                   (currentY+1.f - i2.edge->b)*i2.edge->m;
        return x1 < x2;
    }
}

#ifdef QT_USE_FIXED_POINT
inline int qrealToXFixed(qreal f)
{ return f.value() << 8; }
#else
#define qrealToXFixed FloatToXFixed
#endif

static XTrapezoid QT_FASTCALL toXTrapezoid(XFixed y1, XFixed y2, const QEdge &left, const QEdge &right)
{
    XTrapezoid trap;
    trap.top = y1;
    trap.bottom = y2;
    trap.left.p1.y = left.p1.y;
    trap.left.p2.y = left.p2.y;
    trap.right.p1.y = right.p1.y;
    trap.right.p2.y = right.p2.y;
    trap.left.p1.x = left.p1.x;
    trap.left.p2.x = left.p2.x;
    trap.right.p1.x = right.p1.x;
    trap.right.p2.x = right.p2.x;
    return trap;
}

#ifdef QT_DEBUG_TESSELATOR
static void dump_edges(const QList<const QEdge *> &et)
{
    for (int x = 0; x < et.size(); ++x) {
        qDebug() << "edge#" << x << et.at(x)->p1 << et.at(x)->p2 << "b: " << et.at(x)->b << "m:" << et.at(x)->m << et.at(x);
    }
}

static void dump_trap(const XTrapezoid &t)
{
    qDebug() << "trap# t=" << t.top/65536.0 << "b=" << t.bottom/65536.0  << "h="
             << XFixedToDouble(t.bottom - t.top) << "\tleft p1: ("
             << XFixedToDouble(t.left.p1.x) << ","<< XFixedToDouble(t.left.p1.y)
             << ")" << "\tleft p2: (" << XFixedToDouble(t.left.p2.x) << ","
             << XFixedToDouble(t.left.p2.y) << ")" << "\n\t\t\t\tright p1:("
             << XFixedToDouble(t.right.p1.x) << "," << XFixedToDouble(t.right.p1.y) << ")"
             << "\tright p2:(" << XFixedToDouble(t.right.p2.x) << ","
             << XFixedToDouble(t.right.p2.y) << ")";
}
#endif


void qt_tesselate_polygon(QVector<XTrapezoid> *traps, const QPointF *pg, int pgSize,
                                 bool winding)
{
    QVector<QEdge> edges;
    edges.reserve(128);
    qreal ymin(INT_MAX/256);
    qreal ymax(INT_MIN/256);

    //painter.begin(pg, pgSize);
    Q_ASSERT(pg[0] == pg[pgSize-1]);
    // generate edge table
    for (int x = 0; x < pgSize-1; ++x) {
	QEdge edge;
	edge.winding = pg[x].y() > pg[x+1].y() ? 1 : -1;
        QPointF p1, p2;
	if (edge.winding > 0) {
	    p1 = pg[x+1];
	    p2 = pg[x];
	} else {
	    p1 = pg[x];
	    p2 = pg[x+1];
	}
        edge.p1.x = XDoubleToFixed(p1.x());
        edge.p1.y = XDoubleToFixed(p1.y());
        edge.p2.x = XDoubleToFixed(p2.x());
        edge.p2.y = XDoubleToFixed(p2.y());

	edge.m = (p1.y() - p2.y()) / (p1.x() - p2.x()); // line derivative
	edge.b = p1.y() - edge.m * p1.x(); // intersection with y axis
	edge.m = edge.m != 0.0 ? 1.0 / edge.m : 0.0; // inverted derivative
	edges.append(edge);
        ymin = qMin(ymin, XFixedToDouble(edge.p1.y));
        ymax = qMax(ymax, XFixedToDouble(edge.p2.y));
    }

    QList<const QEdge *> et; 	    // edge list
    for (int i = 0; i < edges.size(); ++i)
        et.append(&edges.at(i));

    // sort edge table by min y value
    qSort(et.begin(), et.end(), compareEdges);

    // eliminate shared edges
    for (int i = 0; i < et.size(); ++i) {
	for (int k = i+1; k < et.size(); ++k) {
            const QEdge *edgeI = et.at(i);
            const QEdge *edgeK = et.at(k);
            if (edgeK->p1.y > edgeI->p1.y)
                break;
   	    if (edgeI->winding != edgeK->winding &&
                isEqual(edgeI->p1, edgeK->p1) && isEqual(edgeI->p2, edgeK->p2)
		) {
 		et.removeAt(k);
		et.removeAt(i);
		--i;
		break;
	    }
	}
    }

    if (ymax <= ymin)
	return;
    QList<const QEdge *> aet; 	    // edges that intersects the current scanline

//     if (ymin < 0)
// 	ymin = 0;
//     if (paintEventClipRegion) // don't scan more lines than we have to
// 	ymax = paintEventClipRegion->boundingRect().height();

#ifdef QT_DEBUG_TESSELATOR
    qDebug("==> ymin = %f, ymax = %f", ymin, ymax);
#endif // QT_DEBUG_TESSELATOR

    currentY = ymin; // used by the less than op
    for (qreal y = ymin; y < ymax;) {
	// fill active edge table with edges that intersect the current line
	for (int i = 0; i < et.size(); ++i) {
            const QEdge *edge = et.at(i);
            if (edge->p1.y > XDoubleToFixed(y))
                break;
            aet.append(edge);
            et.removeAt(i);
            --i;
	}

	// remove processed edges from active edge table
	for (int i = 0; i < aet.size(); ++i) {
	    if (aet.at(i)->p2.y <= XDoubleToFixed(y)) {
		aet.removeAt(i);
 		--i;
	    }
	}
        if (aet.size()%2 != 0) {
#ifndef QT_NO_DEBUG
            qWarning("QX11PaintEngine: aet out of sync - this should not happen.");
#endif
            return;
        }

	// done?
	if (!aet.size()) {
            if (!et.size()) {
                break;
	    } else {
 		y = XFixedToDouble(et.at(0)->p1.y);
                continue;
	    }
        }

        // calculate the next y where we have to start a new set of trapezoids
	qreal next_y(INT_MAX/256);
 	for (int i = 0; i < aet.size(); ++i) {
            const QEdge *edge = aet.at(i);
 	    if (XFixedToDouble(edge->p2.y) < next_y)
 		next_y = XFixedToDouble(edge->p2.y);
        }

	if (et.size() && next_y > XFixedToDouble(et.at(0)->p1.y))
	    next_y = XFixedToDouble(et.at(0)->p1.y);

        int aetSize = aet.size();
	for (int i = 0; i < aetSize; ++i) {
	    for (int k = i+1; k < aetSize; ++k) {
                const QEdge *edgeI = aet.at(i);
                const QEdge *edgeK = aet.at(k);
		qreal m1 = edgeI->m;
		qreal b1 = edgeI->b;
		qreal m2 = edgeK->m;
		qreal b2 = edgeK->b;

		if (qAbs(m1 - m2) < 0.001)
                    continue;

                // ### intersect is not calculated correctly when optimized with -O2 (gcc)
                volatile qreal intersect = 0;
                if (!qIsFinite(b1))
                    intersect = (1.f / m2) * XFixedToDouble(edgeI->p1.x) + b2;
                else if (!qIsFinite(b2))
                    intersect = (1.f / m1) * XFixedToDouble(edgeK->p1.x) + b1;
                else
                    intersect = (b1*m1 - b2*m2) / (m1 - m2);

 		if (intersect > y && intersect < next_y)
		    next_y = intersect;
	    }
	}

        XFixed yf, next_yf;
        yf = qrealToXFixed(y);
        next_yf = qrealToXFixed(next_y);

        if (yf == next_yf) {
            y = currentY = next_y;
            continue;
        }

#ifdef QT_DEBUG_TESSELATOR
        qDebug("###> y = %f, next_y = %f, %d active edges", y, next_y, aet.size());
        qDebug("===> edges");
        dump_edges(et);
        qDebug("===> active edges");
        dump_edges(aet);
#endif
	// calc intersection points
 	QVarLengthArray<QIntersectionPoint> isects(aet.size()+1);
 	for (int i = 0; i < isects.size()-1; ++i) {
            const QEdge *edge = aet.at(i);
 	    isects[i].x = (edge->p1.x != edge->p2.x) ?
			  ((y - edge->b)*edge->m) : XFixedToDouble(edge->p1.x);
	    isects[i].edge = edge;
	}

	Q_ASSERT(isects.size()%2 == 1);

	// sort intersection points
 	qSort(&isects[0], &isects[isects.size()-1], compareIntersections);

        if (winding) {
            // winding fill rule
            for (int i = 0; i < isects.size()-1;) {
                int winding = 0;
                const QEdge *left = isects[i].edge;
                const QEdge *right = 0;
                winding += isects[i].edge->winding;
                for (++i; i < isects.size()-1 && winding != 0; ++i) {
                    winding += isects[i].edge->winding;
                    right = isects[i].edge;
                }
                if (!left || !right)
                    break;
                //painter.addTrapezoid(&toXTrapezoid(yf, next_yf, *left, *right));
                traps->append(toXTrapezoid(yf, next_yf, *left, *right));
            }
        } else {
            // odd-even fill rule
            for (int i = 0; i < isects.size()-2; i += 2) {
                //painter.addTrapezoid(&toXTrapezoid(yf, next_yf, *isects[i].edge, *isects[i+1].edge));
                traps->append(toXTrapezoid(yf, next_yf, *isects[i].edge, *isects[i+1].edge));
            }
        }
	y = currentY = next_y;
    }

#ifdef QT_DEBUG_TESSELATOR
    qDebug("==> number of trapezoids: %d - edge table size: %d\n", traps->size(), et.size());

    for (int i = 0; i < traps->size(); ++i)
        dump_trap(traps->at(i));
#endif

    // optimize by unifying trapezoids that share left/right lines
    // and have a common top/bottom edge
//     for (int i = 0; i < tps.size(); ++i) {
// 	for (int k = i+1; k < tps.size(); ++k) {
// 	    if (i != k && tps.at(i).right == tps.at(k).right
// 		&& tps.at(i).left == tps.at(k).left
// 		&& (tps.at(i).top == tps.at(k).bottom
// 		    || tps.at(i).bottom == tps.at(k).top))
// 	    {
// 		tps[i].bottom = tps.at(k).bottom;
// 		tps.removeAt(k);
//                 i = 0;
// 		break;
// 	    }
// 	}
//     }
    //static int i = 0;
    //QImage img = painter.end();
    //img.save(QString("res%1.png").arg(i++), "PNG");
}
