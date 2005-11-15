/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QHEADERVIEW_P_H
#define QHEADERVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qabstractitemview_p.h"

#ifndef QT_NO_ITEMVIEWS
#include "QtCore/qbitarray.h"
#include "QtGui/qapplication.h"
#include "QtGui/qlabel.h"

class QHeaderViewPrivate: public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QHeaderView)

public:
    QHeaderViewPrivate()
        : state(NoState),
          offset(0),
          sortIndicatorOrder(Qt::AscendingOrder),
          sortIndicatorSection(0),
          sortIndicatorShown(false),
          lastPos(-1),
          firstPos(-1),
          section(-1),
          target(-1),
          pressed(-1),
          hover(-1),
          movableSections(false),
          clickableSections(false),
          highlightSelected(false),
          stretchLastSection(false),
          stretchSections(0),
          sectionIndicatorOffset(0),
          sectionIndicator(0),
          globalResizeMode(QHeaderView::Interactive) {}

    int sectionHandleAt(int position);
    void setupSectionIndicator(int section, int position);
    void updateSectionIndicator(int section, int position);
    void resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode = false);

    bool isSectionSelected(int section) const;
    inline void prepareSectionSelected() {
        if (sectionSelection.count() != sections.count() * 2)
            sectionSelection.fill(false, sections.count() * 2);
        else sectionSelection.fill(false);
    }

    inline bool reverse() const {
        return q_func()->isRightToLeft() && orientation == Qt::Horizontal;
    }

    inline int logicalIndex(int visualIndex) const {
        return logicalIndices.isEmpty() ? visualIndex : logicalIndices.at(visualIndex);
    }

    inline void setDefaultValues(Qt::Orientation o) {
        defaultSectionSize = (o == Qt::Horizontal ? 100 : 30);
        defaultAlignment = (o == Qt::Horizontal
                            ? Qt::Alignment(Qt::AlignCenter)
                            : Qt::AlignLeft|Qt::AlignVCenter);
        orientation = o;
    }

    inline bool isVisualIndexHidden(int visual) const {
        return !sectionHidden.isEmpty() && sectionHidden.at(visual);
    }

    inline void setVisualIndexHidden(int visual, bool hidden) {
        if (!sectionHidden.isEmpty()) sectionHidden.setBit(visual, hidden);
    }

    enum State { NoState, ResizeSection, MoveSection } state;

    int offset;
    Qt::Orientation orientation;
    Qt::SortOrder sortIndicatorOrder;
    int sortIndicatorSection;
    bool sortIndicatorShown;

    struct HeaderSection {
        int position;
        QHeaderView::ResizeMode mode;
    };

    void clear() {
        sections.clear();
        visualIndices.clear();
        logicalIndices.clear();
        sectionSelection.clear();
        sectionHidden.clear();
        hiddenSectionSize.clear();
    }
    mutable QVector<HeaderSection> sections; // HeaderSection = sections.at(visualIndex)
    mutable QVector<int> visualIndices; // visualIndex = visualIndices.at(logicalIndex)
    mutable QVector<int> logicalIndices; // logicalIndex = row or column in the model
    mutable QBitArray sectionSelection;
    mutable QBitArray sectionHidden;
    mutable QHash<int, int> hiddenSectionSize; // from logical index to section size

    int lastPos;
    int firstPos;
    int section; // used for resizing and moving sections
    int target;
    int pressed;
    int hover;
    bool movableSections;
    bool clickableSections;
    bool highlightSelected;
    bool stretchLastSection;
    int stretchSections;
    int sectionIndicatorOffset;
    int defaultSectionSize;
    Qt::Alignment defaultAlignment;
    QLabel *sectionIndicator;
    QStyleOptionHeader getStyleOption() const;
    QHeaderView::ResizeMode globalResizeMode;
};

#endif // QT_NO_ITEMVIEWS

#endif // QHEADERVIEW_P_H
