#ifndef QGENERICCOMBOBOX_P_H
#define QGENERICCOMBOBOX_P_H

#ifndef QT_H
#include <qgenericlistview.h>
#include <qlineedit.h>
#include <qgenericcombobox.h>
#include <qbasictimer.h>
#include <qabstractslider.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qpainter.h>
#include <qitemdelegate.h>
#include <qapplication.h>
#include <private/qwidget_p.h>
#endif // QT_H

class Scroller : public QWidget
{
    Q_OBJECT

public:
    Scroller(QAbstractSlider::SliderAction action, QWidget *parent)
        : QWidget(parent), sliderAction(action) {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    }
    QSize sizeHint() const {
        return QSize(20, style().pixelMetric(QStyle::PM_MenuScrollerHeight));
    }

protected:
    void enterEvent(QEvent *) {
        timer.start(100, this);
    }
    void leaveEvent(QEvent *) {
        timer.stop();
    }
    void timerEvent(QTimerEvent *e) {
        if (e->timerId() == timer.timerId())
            emit doScroll(sliderAction);
    }
    void hideEvent(QHideEvent *) {
        timer.stop();
    }
    void paintEvent(QEvent *) {
        QPainter p(this);
        Q4StyleOptionMenuItem menuOpt(0);
        menuOpt.palette = palette();
        menuOpt.state = QStyle::Style_Default;
        menuOpt.checkState = Q4StyleOptionMenuItem::NotCheckable;
        menuOpt.menuRect = rect();
        menuOpt.maxIconWidth = 0;
        menuOpt.tabWidth = 0;
        menuOpt.menuItemType = Q4StyleOptionMenuItem::Scroller;
        if (sliderAction == QAbstractSlider::SliderSingleStepAdd)
            menuOpt.state = QStyle::Style_Down;
        menuOpt.rect = rect();
        style().drawControl(QStyle::CE_MenuScroller, &menuOpt, &p);
    }

signals:
    void doScroll(int action);

private:
    QAbstractSlider::SliderAction sliderAction;
    QBasicTimer timer;
};

class ListViewContainer : public QFrame
{
    Q_OBJECT

public:
    ListViewContainer(QGenericListView *listView, QWidget *parent = 0);
    QGenericListView *listView() const;

public slots:
    void scrollListView(int action);
    void updateScrollers();

protected:
    bool eventFilter(QObject *o, QEvent *e);
    void keyPressEvent(QKeyEvent *e);

signals:
    void itemSelected(const QModelIndex &);

private:
    QGenericListView *list;
    Scroller *top;
    Scroller *bottom;
};

class MenuDelegate : public QItemDelegate
{
public:
    MenuDelegate(QAbstractItemModel *model, QObject *parent) : QItemDelegate(model, parent) {}

protected:
    void paint(QPainter *painter, const QItemOptions &options,
               const QModelIndex &index) const
        {
            Q4StyleOptionMenuItem opt = getStyleOption(options, index);
            QApplication::style().drawControl(QStyle::CE_MenuItem, &opt, painter, 0);
        }
    QSize sizeHint(const QFontMetrics &fontMetrics, const QItemOptions &options,
                   const QModelIndex &index) const
        {
            Q4StyleOptionMenuItem opt = getStyleOption(options, index);
            return QApplication::style().sizeFromContents(
                QStyle::CT_MenuItem, &opt, options.itemRect.size(), fontMetrics, 0);
        }

private:
    Q4StyleOptionMenuItem getStyleOption(const QItemOptions &options, const QModelIndex &index) const
        {
            Q4StyleOptionMenuItem opt(0);
            opt.palette = options.palette;
            opt.state = QStyle::Style_Default;
            if (options.disabled)
                opt.palette.setCurrentColorGroup(QPalette::Disabled);
            else
                opt.state |= QStyle::Style_Enabled;
            opt.state |= QStyle::Style_ButtonDefault;
            if (options.selected) {
                opt.state |= QStyle::Style_Active;
                opt.checkState = Q4StyleOptionMenuItem::Checked;
            } else {
                opt.checkState = Q4StyleOptionMenuItem::Unchecked;
            }
            opt.menuItemType = Q4StyleOptionMenuItem::Normal;
            opt.icon = model()->data(index, QAbstractItemModel::Decoration).toIconSet();
            opt.text = model()->data(index, QAbstractItemModel::Display).toString();
            opt.tabWidth = 0;
            opt.maxIconWidth = 0;
            if (!opt.icon.isNull())
                opt.maxIconWidth = opt.icon.pixmap(QIconSet::Small, QIconSet::Normal).width() + 4;
            opt.menuRect = options.itemRect;
            opt.rect = options.itemRect;
            return opt;
        }
};

class QGenericComboBoxPrivate: public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QGenericComboBox)
public:
    QGenericComboBoxPrivate()
        : QWidgetPrivate(),
          model(0),
          lineEdit(0),
          delegate(0),
          container(0),
          insertionPolicy(QGenericComboBox::AtBottom),
          autoCompletion(true),
          duplicatesEnabled(false),
          sizeLimit(10),
          ignoreMousePressEvent(false),
          skipCompletion(false) {}
    ~QGenericComboBoxPrivate() {}
    void init();
    Q4StyleOptionComboBox getStyleOption() const;
    void updateLineEditGeometry();
    void returnPressed();
    void complete();
    void itemSelected(const QModelIndex &item);
    bool contains(const QString &text, int role);

    QAbstractItemModel *model;
    QLineEdit *lineEdit;
    QAbstractItemDelegate *delegate;
    ListViewContainer *container;
    QGenericComboBox::InsertionPolicy insertionPolicy;
    bool autoCompletion;
    bool duplicatesEnabled;
    int sizeLimit;
    bool ignoreMousePressEvent;
    bool skipCompletion;
    mutable QSize sizeHint;
    QPersistentModelIndex currentItem;
    QPersistentModelIndex root;
};

#endif //QGENERICCOMBOBOX_P_H
