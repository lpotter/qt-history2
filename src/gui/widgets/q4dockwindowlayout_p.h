#ifndef QDOCKWINDOWLAYOUT_P_H
#define QDOCKWINDOWLAYOUT_P_H

#include <qlayout.h>
#include <qlist.h>

class QDockWindow;
class QDockWindowSeparator;

struct QDockWindowLayoutInfo
{
    QLayoutItem *item;

    QCOORD cur_pos;
    QCOORD cur_size;
    QCOORD min_size;
    QCOORD max_size;

    uint is_sep     : 1;
    uint is_dummy   : 1;
    uint is_dropped : 1;
    uint is_act     : 1; // active tab?

    inline QDockWindowLayoutInfo(QLayoutItem *i)
	: item(i), cur_pos(-1), cur_size(-1), min_size(1), max_size(~0u),
	  is_sep(0), is_dummy(0), is_dropped(0), is_act(0)
    { }
};

class QDockWindowLayout : public QLayout
{
    Q_OBJECT

 public:
    Qt::Orientation orientation;
    QList<QDockWindowLayoutInfo> layout_info;
    QList<QDockWindowLayoutInfo> *save_layout_info;

    QDockWindowLayout(QWidget *widget, Orientation o);
    QDockWindowLayout(QLayout *layout, Orientation o);

    // QLayout interface
    void addItem(QLayoutItem *layoutitem);
    void setGeometry(const QRect &rect);
    QLayoutItem *itemAt(int index) const;
    QLayoutItem *takeAt(int index);
    QSize sizeHint() const;
    QSize minimumSize() const;
    void invalidate();
    bool isEmpty() const;

    QInternal::RelayoutType relayout_type;
    void relayout(QInternal::RelayoutType type = QInternal::RelayoutNormal);

    void setOrientation(Qt::Orientation o);
    QLayoutItem *find(QWidget *widget);
    QDockWindowLayoutInfo &insert(int index, QLayoutItem *layoutitem, bool dummy = false);

    void dump();

    void saveLayoutInfo();
    void resetLayoutInfo();
    void discardLayoutInfo();

    QPoint constrain(QDockWindowSeparator *sep, int delta);

    struct Location {
        int index;
        DockWindowArea area;
    };
    Location locate(const QPoint &mouse) const;
    QRect place(QDockWindow *dockwindow, const QRect &r, const QPoint &mouse);
    void drop(QDockWindow *dockwindow, const QRect &r, const QPoint &mouse);

    void extend(QDockWindow *dockwindow, Orientation direction);
    void split(QDockWindow *dockwindow, Orientation direction);

    void split(QDockWindow *existing, QDockWindow *with, DockWindowArea area);
};

static inline QCOORD pick(Qt::Orientation o, const QPoint &p)
{ return o == Qt::Horizontal ? p.x() : p.y(); }
static inline QCOORD pick(Qt::Orientation o, const QSize &s)
{ return o == Qt::Horizontal ? s.width() : s.height(); }
static inline QCOORD pick_perp(Qt::Orientation o, const QPoint &p)
{ return o == Qt::Vertical ? p.x() : p.y(); }
static inline QCOORD pick_perp(Qt::Orientation o, const QSize &s)
{ return o == Qt::Vertical ? s.width() : s.height(); }

#endif // QDOCKWINDOWLAYOUT_P_H
