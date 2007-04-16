/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QFont>
#include <QtTest/QtTest>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWizard>
#include <QStyle>

//TESTED_CLASS=QWizard
//TESTED_FILES=gui/dialogs/qwizard.h gui/dialogs/qwizard.cpp

static QImage grabWidget(QWidget *window)
{
    return QPixmap::grabWidget(window).toImage();
}

class tst_QWizard : public QObject
{
    Q_OBJECT

public:
    tst_QWizard();

public slots:
    void init();
    void cleanup();

private slots:
    void buttonText();
    void setButtonLayout();
    void setButton();
    void setTitleFormatEtc();
    void setPixmap();
    void setDefaultProperty();
    void addPage();
    void setPage();
    void setStartId();
    void setOption_IndependentPages();
    void setOption_IgnoreSubTitles();
    void setOption_ExtendedWatermarkPixmap();
    void setOption_NoDefaultButton();
    void setOption_NoBackButtonOnStartPage();
    void setOption_NoBackButtonOnLastPage();
    void setOption_DisabledBackButtonOnLastPage();
    void setOption_HaveNextButtonOnLastPage();
    void setOption_HaveFinishButtonOnEarlyPages();
    void setOption_NoCancelButton();
    void setOption_CancelButtonOnLeft();
    void setOption_HaveHelpButton();
    void setOption_HelpButtonOnRight();
    void setOption_HaveCustomButtonX();
    void combinations_data();
    void combinations();
    void showCurrentPageOnly();
    void setButtonText();
    void setCommitPage();
    void setWizardStyle();

    /*
        Things that could be added:

        1. Test virtual functions that are called, signals that are
           emitted, etc.

        2. Test QWizardPage more thorougly.

        3. Test the look and field a bit more (especially the
           different wizard styles, and how they interact with
           pixmaps, titles, subtitles, etc.).

        4. Test minimum sizes, sizes, maximum sizes, resizing, etc.

        5. Try setting various options and wizard styles in various
           orders and check that the results are the same every time,
           no matter the order in which the properties were set.

           -> Initial version done (tst_QWizard::combinations())

        6. Test done() and restart().

        7. Test default properties of built-in widgets.

        8. Test mutual exclusiveness of Next and Commit buttons.
    */
};

tst_QWizard::tst_QWizard()
{
}

void tst_QWizard::init()
{
}

void tst_QWizard::cleanup()
{
}

void tst_QWizard::buttonText()
{
    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);

    // Check the buttons' original text in Classic and Modern styles.
    for (int pass = 0; pass < 2; ++pass) {
        QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("< &Back"));
        QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
        QVERIFY(wizard.buttonText(QWizard::FinishButton).endsWith("Finish"));
        QVERIFY(wizard.buttonText(QWizard::CancelButton).endsWith("Cancel"));
        QVERIFY(wizard.buttonText(QWizard::HelpButton).endsWith("Help"));

        QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::CustomButton2).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

        // robustness
        QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

        wizard.setWizardStyle(QWizard::ModernStyle);
    }

    // Check the buttons' original text in Mac style.
    wizard.setWizardStyle(QWizard::MacStyle);

    QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("Go Back"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("Continue"));
    QCOMPARE(wizard.buttonText(QWizard::FinishButton), QString("Done"));
    QCOMPARE(wizard.buttonText(QWizard::CancelButton), QString("Quit"));
    QCOMPARE(wizard.buttonText(QWizard::HelpButton), QString("Help"));

    QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::CustomButton2).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

    // robustness
    QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

    // Modify the buttons' text and see what happens.
    wizard.setButtonText(QWizard::NextButton, "N&este");
    wizard.setButtonText(QWizard::CustomButton2, "&Cucu");
    wizard.setButtonText(QWizard::Stretch, "Stretch");

    QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("Go Back"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("N&este"));
    QCOMPARE(wizard.buttonText(QWizard::FinishButton), QString("Done"));
    QCOMPARE(wizard.buttonText(QWizard::CancelButton), QString("Quit"));
    QCOMPARE(wizard.buttonText(QWizard::HelpButton), QString("Help"));

    QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
    QCOMPARE(wizard.buttonText(QWizard::CustomButton2), QString("&Cucu"));
    QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

    // robustness
    QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
    QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

    // Switch back to Classic style and see what happens.
    wizard.setWizardStyle(QWizard::ClassicStyle);

    for (int pass = 0; pass < 2; ++pass) {
        QCOMPARE(wizard.buttonText(QWizard::BackButton), QString("< &Back"));
        QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("N&este"));
        QVERIFY(wizard.buttonText(QWizard::FinishButton).endsWith("Finish"));
        QVERIFY(wizard.buttonText(QWizard::CancelButton).endsWith("Cancel"));
        QVERIFY(wizard.buttonText(QWizard::HelpButton).endsWith("Help"));

        QVERIFY(wizard.buttonText(QWizard::CustomButton1).isEmpty());
        QCOMPARE(wizard.buttonText(QWizard::CustomButton2), QString("&Cucu"));
        QVERIFY(wizard.buttonText(QWizard::CustomButton3).isEmpty());

        // robustness
        QVERIFY(wizard.buttonText(QWizard::Stretch).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NoButton).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NStandardButtons).isEmpty());
        QVERIFY(wizard.buttonText(QWizard::NButtons).isEmpty());

        wizard.setOptions(QWizard::NoDefaultButton
                          | QWizard::NoBackButtonOnStartPage
                          | QWizard::NoBackButtonOnLastPage
                          | QWizard::DisabledBackButtonOnLastPage
                          | QWizard::NoCancelButton
                          | QWizard::CancelButtonOnLeft
                          | QWizard::HaveHelpButton
                          | QWizard::HelpButtonOnRight
                          | QWizard::HaveCustomButton1
                          | QWizard::HaveCustomButton2
                          | QWizard::HaveCustomButton3);
    }
}

void tst_QWizard::setButtonLayout()
{
    QList<QWizard::WizardButton> layout;

    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    wizard.setOptions(0);
    wizard.setButtonLayout(layout);
    wizard.show();
    qApp->processEvents();

    // if these crash, this means there's a bug in QWizard
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.button(QWizard::BackButton)->text().contains("Back"));
    QVERIFY(wizard.button(QWizard::FinishButton)->text().contains("Finish"));
    QVERIFY(wizard.button(QWizard::CancelButton)->text().contains("Cancel"));
    QVERIFY(wizard.button(QWizard::HelpButton)->text().contains("Help"));
    QVERIFY(wizard.button(QWizard::CustomButton1)->text().isEmpty());
    QVERIFY(wizard.button(QWizard::CustomButton2)->text().isEmpty());
    QVERIFY(wizard.button(QWizard::CustomButton3)->text().isEmpty());
    QVERIFY(!wizard.button(QWizard::Stretch));
    QVERIFY(!wizard.button(QWizard::NoButton));

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

    layout << QWizard::NextButton << QWizard::HelpButton;
    wizard.setButtonLayout(layout);
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    wizard.restart();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

    layout.clear();
    layout << QWizard::NextButton << QWizard::HelpButton << QWizard::BackButton
           << QWizard::FinishButton << QWizard::CancelButton << QWizard::Stretch
           << QWizard::CustomButton2;

    // Turn on all the button-related wizard options. Some of these
    // should have no impact on a custom layout; others should.
    wizard.setButtonLayout(layout);
    wizard.setOptions(QWizard::NoDefaultButton
                      | QWizard::NoBackButtonOnStartPage
                      | QWizard::NoBackButtonOnLastPage
                      | QWizard::DisabledBackButtonOnLastPage
                      | QWizard::HaveNextButtonOnLastPage
                      | QWizard::HaveFinishButtonOnEarlyPages
                      | QWizard::NoCancelButton
                      | QWizard::CancelButtonOnLeft
                      | QWizard::HaveHelpButton
                      | QWizard::HelpButtonOnRight
                      | QWizard::HaveCustomButton1
                      | QWizard::HaveCustomButton2
                      | QWizard::HaveCustomButton3);
    qApp->processEvents();

    // we're on first page
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());
    QVERIFY(wizard.button(QWizard::CancelButton)->isVisible()); // NoCancelButton overridden
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::CustomButton1)->isVisible());
    QVERIFY(wizard.button(QWizard::CustomButton2)->isVisible());    // HaveCustomButton2 overridden
    QVERIFY(!wizard.button(QWizard::CustomButton3)->isVisible());

    wizard.next();
    qApp->processEvents();

    // we're on last page
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());
    QVERIFY(wizard.button(QWizard::FinishButton)->isEnabled());
    QVERIFY(wizard.button(QWizard::CancelButton)->isVisible()); // NoCancelButton overridden
    QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::CustomButton1)->isVisible());
    QVERIFY(wizard.button(QWizard::CustomButton2)->isVisible());    // HaveCustomButton2 overridden
    QVERIFY(!wizard.button(QWizard::CustomButton3)->isVisible());

    // Check that the buttons are in the right order on screen.
    for (int pass = 0; pass < 2; ++pass) {
        wizard.setLayoutDirection(pass == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        qApp->processEvents();

        int sign = (pass == 0) ? +1 : -1;

        int p[5];
        p[0] = sign * wizard.button(QWizard::NextButton)->x();
        p[1] = sign * wizard.button(QWizard::HelpButton)->x();
        p[2] = sign * wizard.button(QWizard::FinishButton)->x();
        p[3] = sign * wizard.button(QWizard::CancelButton)->x();
        p[4] = sign * wizard.button(QWizard::CustomButton2)->x();

        QVERIFY(p[0] < p[1]);
        QVERIFY(p[1] < p[2]);
        QVERIFY(p[2] < p[3]);
        QVERIFY(p[3] < p[4]);
    }

    layout.clear();
    wizard.setButtonLayout(layout);
    qApp->processEvents();

    for (int i = -1; i < 50; ++i) {
        QAbstractButton *button = wizard.button(QWizard::WizardButton(i));
        QVERIFY(!button || !button->isVisible());
    }
}

void tst_QWizard::setButton()
{
    QPointer<QToolButton> toolButton = new QToolButton;

    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    wizard.setButton(QWizard::NextButton, toolButton);
    wizard.setButton(QWizard::CustomButton2, new QCheckBox("Kustom 2"));

    QVERIFY(qobject_cast<QToolButton *>(wizard.button(QWizard::NextButton)));
    QVERIFY(qobject_cast<QCheckBox *>(wizard.button(QWizard::CustomButton2)));
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::CustomButton1)));

    QVERIFY(toolButton != 0);

    // resetting the same button does nothing
    wizard.setButton(QWizard::NextButton, toolButton);
    QVERIFY(toolButton != 0);

    // revert to default button
    wizard.setButton(QWizard::NextButton, 0);
    QVERIFY(toolButton == 0);
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton)));
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
}

void tst_QWizard::setTitleFormatEtc()
{
    QWizard wizard;
    QVERIFY(wizard.titleFormat() == Qt::AutoText);
    QVERIFY(wizard.subTitleFormat() == Qt::AutoText);

    wizard.setTitleFormat(Qt::RichText);
    QVERIFY(wizard.titleFormat() == Qt::RichText);
    QVERIFY(wizard.subTitleFormat() == Qt::AutoText);

    wizard.setSubTitleFormat(Qt::PlainText);
    QVERIFY(wizard.titleFormat() == Qt::RichText);
    QVERIFY(wizard.subTitleFormat() == Qt::PlainText);
}

void tst_QWizard::setPixmap()
{
    QPixmap p1(1, 1);
    QPixmap p2(2, 2);
    QPixmap p3(3, 3);
    QPixmap p4(4, 4);
    QPixmap p5(5, 5);

    QWizard wizard;
    QWizardPage *page = new QWizardPage;
    QWizardPage *page2 = new QWizardPage;

    wizard.addPage(page);
    wizard.addPage(page2);

    QVERIFY(wizard.pixmap(QWizard::BannerPixmap).isNull());
    QVERIFY(wizard.pixmap(QWizard::LogoPixmap).isNull());
    QVERIFY(wizard.pixmap(QWizard::WatermarkPixmap).isNull());
#ifdef Q_WS_MAC
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_3)
        QVERIFY(wizard.pixmap(QWizard::BackgroundPixmap).isNull() == false);
    else  // fall through since the image doesn't exist on a 10.3 system.
#else
    QVERIFY(wizard.pixmap(QWizard::BackgroundPixmap).isNull());
#endif

    QVERIFY(page->pixmap(QWizard::BannerPixmap).isNull());
    QVERIFY(page->pixmap(QWizard::LogoPixmap).isNull());
    QVERIFY(page->pixmap(QWizard::WatermarkPixmap).isNull());
#ifdef Q_WS_MAC
    QVERIFY(page->pixmap(QWizard::BackgroundPixmap).isNull() == false);
#else
    QVERIFY(page->pixmap(QWizard::BackgroundPixmap).isNull());
#endif
    wizard.setPixmap(QWizard::BannerPixmap, p1);
    wizard.setPixmap(QWizard::LogoPixmap, p2);
    wizard.setPixmap(QWizard::WatermarkPixmap, p3);
    wizard.setPixmap(QWizard::BackgroundPixmap, p4);

    page->setPixmap(QWizard::LogoPixmap, p5);

    QVERIFY(wizard.pixmap(QWizard::BannerPixmap).size() == p1.size());
    QVERIFY(wizard.pixmap(QWizard::LogoPixmap).size() == p2.size());
    QVERIFY(wizard.pixmap(QWizard::WatermarkPixmap).size() == p3.size());
    QVERIFY(wizard.pixmap(QWizard::BackgroundPixmap).size() == p4.size());

    QVERIFY(page->pixmap(QWizard::BannerPixmap).size() == p1.size());
    QVERIFY(page->pixmap(QWizard::LogoPixmap).size() == p5.size());
    QVERIFY(page->pixmap(QWizard::WatermarkPixmap).size() == p3.size());
    QVERIFY(page->pixmap(QWizard::BackgroundPixmap).size() == p4.size());

    QVERIFY(page2->pixmap(QWizard::BannerPixmap).size() == p1.size());
    QVERIFY(page2->pixmap(QWizard::LogoPixmap).size() == p2.size());
    QVERIFY(page2->pixmap(QWizard::WatermarkPixmap).size() == p3.size());
    QVERIFY(page2->pixmap(QWizard::BackgroundPixmap).size() == p4.size());
}

class MyPage1 : public QWizardPage
{
public:
    MyPage1() {
        edit1 = new QLineEdit("Bla 1");

        edit2 = new QLineEdit("Bla 2");
        edit2->setInputMask("Mask");

        edit3 = new QLineEdit("Bla 3");
        edit3->setMaxLength(25);

        edit4 = new QLineEdit("Bla 4");
    }

    void registerField(const QString &name, QWidget *widget,
                       const char *property = 0,
                       const char *changedSignal = 0)
        { QWizardPage::registerField(name, widget, property, changedSignal); }

    QLineEdit *edit1;
    QLineEdit *edit2;
    QLineEdit *edit3;
    QLineEdit *edit4;
};

void tst_QWizard::setDefaultProperty()
{
    QWizard wizard;
    MyPage1 *page = new MyPage1;
    wizard.addPage(page);

    page->registerField("edit1", page->edit1);

    wizard.setDefaultProperty("QLineEdit", "inputMask", 0);
    page->registerField("edit2", page->edit2);

    wizard.setDefaultProperty("QLineEdit", "maxLength", 0);
    page->registerField("edit3", page->edit3);

    wizard.setDefaultProperty("QLineEdit", "text", SIGNAL(textChanged(QString)));
    page->registerField("edit3bis", page->edit3);

    wizard.setDefaultProperty("QWidget", "enabled", 0); // less specific, i.e. ignored
    page->registerField("edit4", page->edit4);

    wizard.setDefaultProperty("QLineEdit", "customProperty", 0);
    page->registerField("edit4bis", page->edit4);

    QCOMPARE(wizard.field("edit1").toString(), QString("Bla 1"));
    QCOMPARE(wizard.field("edit2").toString(), page->edit2->inputMask());
    QCOMPARE(wizard.field("edit3").toInt(), 25);
    QCOMPARE(wizard.field("edit3bis").toString(), QString("Bla 3"));
    QCOMPARE(wizard.field("edit4").toString(), QString("Bla 4"));
    QCOMPARE(wizard.field("edit4bis").toString(), QString());

    wizard.setField("edit1", "Alpha");
    wizard.setField("edit2", "Beta");
    wizard.setField("edit3", 50);
    wizard.setField("edit3bis", "Gamma");
    wizard.setField("edit4", "Delta");
    wizard.setField("edit4bis", "Epsilon");

    QCOMPARE(wizard.field("edit1").toString(), QString("Alpha"));
    QVERIFY(wizard.field("edit2").toString().contains("Beta"));
    QCOMPARE(wizard.field("edit3").toInt(), 50);
    QCOMPARE(wizard.field("edit3bis").toString(), QString("Gamma"));
    QCOMPARE(wizard.field("edit4").toString(), QString("Delta"));
    QCOMPARE(wizard.field("edit4bis").toString(), QString("Epsilon"));

    // make sure the data structure is reasonable
    for (int i = 0; i < 200000; ++i) {
        wizard.setDefaultProperty("QLineEdit", "x" + QByteArray::number(i), 0);
        wizard.setDefaultProperty("QLabel", "y" + QByteArray::number(i), 0);
    }
}

void tst_QWizard::addPage()
{
    QWidget *parent = new QWidget;
    QWizard wizard;
    const int N = 100;
    QWizardPage *pages[N];

    for (int i = 0; i < N; ++i) {
        pages[i] = new QWizardPage(parent);
        QCOMPARE(wizard.addPage(pages[i]), i);
        QCOMPARE(pages[i]->window(), &wizard);
        QCOMPARE(wizard.startId(), 0);
    }

    for (int i = 0; i < N; ++i) {
        QVERIFY(pages[i] == wizard.page(i));
    }
    QVERIFY(!wizard.page(-1));
    QVERIFY(!wizard.page(N));
    QVERIFY(!wizard.page(N + 1));

    wizard.setPage(N + 50, new QWizardPage);
    wizard.setPage(-3000, new QWizardPage);

    QWizardPage *pageX = new QWizardPage;
    QCOMPARE(wizard.addPage(pageX), N + 51);
    QCOMPARE(wizard.page(N + 51), pageX);

    QCOMPARE(wizard.addPage(new QWizardPage), N + 52);

    wizard.addPage(0); // generates a warning
}

#define CHECK_VISITED(wizard, list) \
    do { \
        QList<int> myList = list; \
        QCOMPARE((wizard).visitedPages(), myList); \
        Q_FOREACH(int id, myList) \
            QVERIFY((wizard).hasVisitedPage(id)); \
    } while (0)

void tst_QWizard::setPage()
{
    QWidget *parent = new QWidget;
    QWizard wizard;
    QWizardPage *page;

    QCOMPARE(wizard.startId(), -1);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);

    page = new QWizardPage(parent);
    wizard.setPage(-1, page);   // gives a warning and does nothing
    QVERIFY(!wizard.page(-2));
    QVERIFY(!wizard.page(-1));
    QVERIFY(!wizard.page(0));
    QCOMPARE(wizard.startId(), -1);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    page = new QWizardPage(parent);
    wizard.setPage(0, page);
    QCOMPARE(page->window(), &wizard);
    QCOMPARE(wizard.page(0), page);
    QCOMPARE(wizard.startId(), 0);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    page = new QWizardPage(parent);
    wizard.setPage(-2, page);
    QCOMPARE(page->window(), &wizard);
    QCOMPARE(wizard.page(-2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -1);
    QVERIFY(!wizard.currentPage());
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>());

    wizard.restart();
    QCOMPARE(wizard.page(-2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == page);
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    page = new QWizardPage(parent);
    wizard.setPage(2, page);
    QCOMPARE(wizard.page(2), page);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    wizard.restart();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    page = new QWizardPage(parent);
    wizard.setPage(-3, page);
    QCOMPARE(wizard.page(-3), page);
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -2);

    wizard.restart();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -3);
    QVERIFY(wizard.currentPage() == wizard.page(-3));
    QCOMPARE(wizard.nextId(), -2);
    CHECK_VISITED(wizard, QList<int>() << -3);

    wizard.next();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2);

    wizard.next();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), 0);
    QVERIFY(wizard.currentPage() == wizard.page(0));
    QCOMPARE(wizard.nextId(), 2);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0);

    for (int i = 0; i < 100; ++i) {
        wizard.next();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), 2);
        QVERIFY(wizard.currentPage() == wizard.page(2));
        QCOMPARE(wizard.nextId(), -1);
        CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0 << 2);
    }

    wizard.back();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), 0);
    QVERIFY(wizard.currentPage() == wizard.page(0));
    QCOMPARE(wizard.nextId(), 2);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2 << 0);

    wizard.back();
    QCOMPARE(wizard.startId(), -3);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);
    CHECK_VISITED(wizard, QList<int>() << -3 << -2);

    for (int i = 0; i < 100; ++i) {
        wizard.back();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), -3);
        QVERIFY(wizard.currentPage() == wizard.page(-3));
        QCOMPARE(wizard.nextId(), -2);
        CHECK_VISITED(wizard, QList<int>() << -3);
    }

    for (int i = 0; i < 100; ++i) {
        wizard.restart();
        QCOMPARE(wizard.startId(), -3);
        QCOMPARE(wizard.currentId(), -3);
        QVERIFY(wizard.currentPage() == wizard.page(-3));
        QCOMPARE(wizard.nextId(), -2);
        CHECK_VISITED(wizard, QList<int>() << -3);
    }
}

void tst_QWizard::setStartId()
{
    QWizard wizard;
    QCOMPARE(wizard.startId(), -1);

    wizard.setPage(INT_MIN, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(-2, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(0, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(1, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setPage(INT_MAX, new QWizardPage);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setStartId(-1);
    QCOMPARE(wizard.startId(), INT_MIN);

    wizard.setStartId(-2);
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.nextId(), -1);

    wizard.restart();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), -2);
    QVERIFY(wizard.currentPage() == wizard.page(-2));
    QCOMPARE(wizard.nextId(), 0);

    wizard.next();
    QCOMPARE(wizard.startId(), -2);
    QCOMPARE(wizard.currentId(), 0);
    QVERIFY(wizard.currentPage() == wizard.page(0));
    QCOMPARE(wizard.nextId(), 1);

    wizard.setStartId(INT_MIN);
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), 0);
    QVERIFY(wizard.currentPage() == wizard.page(0));
    QCOMPARE(wizard.nextId(), 1);

    wizard.next();
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), 1);
    QVERIFY(wizard.currentPage() == wizard.page(1));
    QCOMPARE(wizard.nextId(), INT_MAX);

    wizard.next();
    QCOMPARE(wizard.startId(), INT_MIN);
    QCOMPARE(wizard.currentId(), INT_MAX);
    QVERIFY(wizard.currentPage() == wizard.page(INT_MAX));
    QCOMPARE(wizard.nextId(), -1);
    CHECK_VISITED(wizard, QList<int>() << -2 << 0 << 1 << INT_MAX);
}

struct MyPage2 : public QWizardPage
{
public:
    MyPage2() : init(0), cleanup(0), validate(0) {}

    void initializePage() { ++init; QWizardPage::initializePage(); checkInvariant(); }
    void cleanupPage() { ++cleanup; QWizardPage::cleanupPage(); checkInvariant(); }
    bool validatePage() { ++validate; return QWizardPage::validatePage(); }

    void check(int init, int cleanup)
    { Q_ASSERT(init == this->init && cleanup == this->cleanup); Q_UNUSED(init); Q_UNUSED(cleanup); }

    int init;
    int cleanup;
    int validate;

private:
    void checkInvariant() { Q_ASSERT(init == cleanup || init - 1 == cleanup); }
};

#define CHECK_PAGE_INIT(i0, c0, i1, c1, i2, c2) \
    page0->check((i0), (c0)); \
    page1->check((i1), (c1)); \
    page2->check((i2), (c2));

void tst_QWizard::setOption_IndependentPages()
{
    MyPage2 *page0 = new MyPage2;
    MyPage2 *page1 = new MyPage2;
    MyPage2 *page2 = new MyPage2;

    QWizard wizard;
    wizard.addPage(page0);
    wizard.addPage(page1);
    wizard.addPage(page2);

    QVERIFY(!wizard.testOption(QWizard::IndependentPages));

    wizard.restart();

    // Make sure initializePage() and cleanupPage() are called are
    // they should when the
    // wizard.testOption(QWizard::IndependentPages option is off.
    for (int i = 0; i < 10; ++i) {
        CHECK_PAGE_INIT(i + 1, i, i, i, i, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i);

        wizard.next();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i, i + 1, i + 1);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i + 1, i + 1, i + 1);

        wizard.back();
        CHECK_PAGE_INIT(i + 1, i, i + 1, i + 1, i + 1, i + 1);

        wizard.restart();
    }

    CHECK_PAGE_INIT(11, 10, 10, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    // Now, turn on the option and check that they're called at the
    // appropiate times (which aren't the same).
    wizard.setOption(QWizard::IndependentPages, true);
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.back();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 10, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.next();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.back();
    CHECK_PAGE_INIT(11, 10, 11, 10, 11, 10);

    wizard.setStartId(2);

    wizard.restart();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.back();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.next();
    CHECK_PAGE_INIT(11, 11, 11, 11, 12, 11);

    wizard.setStartId(0);
    wizard.restart();
    CHECK_PAGE_INIT(12, 11, 11, 11, 12, 12);

    wizard.next();
    CHECK_PAGE_INIT(12, 11, 12, 11, 12, 12);

    wizard.next();
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 12);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 12);

    // Fun stuff here.

    wizard.setOption(QWizard::IndependentPages, false);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.setOption(QWizard::IndependentPages, true);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.setOption(QWizard::IndependentPages, false);
    CHECK_PAGE_INIT(12, 11, 12, 11, 13, 13);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 12, 13, 13);

    wizard.back();
    CHECK_PAGE_INIT(12, 11, 12, 12, 13, 13);
}

void tst_QWizard::setOption_IgnoreSubTitles()
{
    QWizard wizard1;
    wizard1.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard1.resize(500, 500);
    QVERIFY(!wizard1.testOption(QWizard::IgnoreSubTitles));
    QWizardPage *page11 = new QWizardPage;
    page11->setTitle("Page X");
    page11->setSubTitle("Some subtitle");

    QWizardPage *page12 = new QWizardPage;
    page12->setTitle("Page X");

    wizard1.addPage(page11);
    wizard1.addPage(page12);

    QWizard wizard2;
    wizard2.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard2.resize(500, 500);
    wizard2.setOption(QWizard::IgnoreSubTitles, true);
    QWizardPage *page21 = new QWizardPage;
    page21->setTitle("Page X");
    page21->setSubTitle("Some subtitle");

    QWizardPage *page22 = new QWizardPage;
    page22->setTitle("Page X");

    wizard2.addPage(page21);
    wizard2.addPage(page22);

    wizard1.show();
    wizard2.show();

    // Check that subtitles are shown when they should (i.e.,
    // they're set and IgnoreSubTitles is off).

    qApp->setActiveWindow(0); // ensure no focus rectangle around cancel button
    QImage i11 = grabWidget(&wizard1);
    QImage i21 = grabWidget(&wizard2);
    QVERIFY(i11 != i21);

    wizard1.next();
    wizard2.next();

    QImage i12 = grabWidget(&wizard1);
    QImage i22 = grabWidget(&wizard2);
    QVERIFY(i12 == i22);
    QVERIFY(i21 == i22);

    wizard1.back();
    wizard2.back();

    QImage i13 = grabWidget(&wizard1);
    QImage i23 = grabWidget(&wizard2);
    QVERIFY(i13 == i11);
    QVERIFY(i23 == i21);

    wizard1.setOption(QWizard::IgnoreSubTitles, true);
    wizard2.setOption(QWizard::IgnoreSubTitles, false);

    QImage i14 = grabWidget(&wizard1);
    QImage i24 = grabWidget(&wizard2);
    QVERIFY(i14 == i21);
    QVERIFY(i24 == i11);

    // Check the impact of subtitles on the rest of the layout, by
    // using a subtitle that looks empty (but that isn't). In
    // Classic and Modern styles, this should be enough to trigger a
    // "header"; in Mac style, this only creates a QLabel, with no
    // text, i.e. it doesn't affect the layout.

    page11->setSubTitle("<b>&nbsp;</b>");    // not quite empty, but looks empty

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            wizard1.setOption(QWizard::IgnoreSubTitles, j == 0);

            wizard1.setWizardStyle(i == 0 ? QWizard::ClassicStyle
                                   : i == 1 ? QWizard::ModernStyle
                                            : QWizard::MacStyle);
            wizard1.restart();
            QImage i1 = grabWidget(&wizard1);

            wizard1.next();
            QImage i2 = grabWidget(&wizard1);

            if (j == 0 || wizard1.wizardStyle() == QWizard::MacStyle) {
                QVERIFY(i1 == i2);
            } else {
                QVERIFY(i1 != i2);
            }
        }
    }
}

void tst_QWizard::setOption_ExtendedWatermarkPixmap()
{
    QPixmap watermarkPixmap(200, 400);

    QWizard wizard1;
    wizard1.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    QVERIFY(!wizard1.testOption(QWizard::ExtendedWatermarkPixmap));
    QWizardPage *page11 = new QWizardPage;
    page11->setTitle("Page X");
    page11->setPixmap(QWizard::WatermarkPixmap, watermarkPixmap);

    QWizardPage *page12 = new QWizardPage;
    page12->setTitle("Page X");

    wizard1.addPage(page11);
    wizard1.addPage(page12);

    QWizard wizard2;
    wizard2.setButtonLayout(QList<QWizard::WizardButton>() << QWizard::CancelButton);
    wizard2.setOption(QWizard::ExtendedWatermarkPixmap, true);
    QWizardPage *page21 = new QWizardPage;
    page21->setTitle("Page X");
    page21->setPixmap(QWizard::WatermarkPixmap, watermarkPixmap);

    QWizardPage *page22 = new QWizardPage;
    page22->setTitle("Page X");

    wizard2.addPage(page21);
    wizard2.addPage(page22);

    wizard1.show();
    wizard2.show();

    // Check the impact of watermark pixmaps on the rest of the layout.

    for (int i = 0; i < 3; ++i) {
        QImage i1[2];
        QImage i2[2];
        for (int j = 0; j < 2; ++j) {
            wizard1.setOption(QWizard::ExtendedWatermarkPixmap, j == 0);

            wizard1.setWizardStyle(i == 0 ? QWizard::ClassicStyle
                                   : i == 1 ? QWizard::ModernStyle
                                            : QWizard::MacStyle);
            wizard1.restart();
            wizard1.setMaximumSize(1000, 1000);
            wizard1.resize(600, 600);
            i1[j] = grabWidget(&wizard1);

            wizard1.next();
            wizard1.setMaximumSize(1000, 1000);
            wizard1.resize(600, 600);
            i2[j] = grabWidget(&wizard1);
        }

        if (wizard1.wizardStyle() == QWizard::MacStyle) {
            QVERIFY(i1[0] == i1[1]);
            QVERIFY(i2[0] == i2[1]);
            QVERIFY(i1[0] == i2[0]);
        } else {
            QVERIFY(i1[0] != i1[1]);
            QVERIFY(i2[0] == i2[1]);
            QVERIFY(i1[0] != i2[0]);
            QVERIFY(i1[1] != i2[1]);
        }
    }
}

void tst_QWizard::setOption_NoDefaultButton()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoDefaultButton, false);
    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
    wizard.addPage(new QWizardPage);
    wizard.page(0)->setFinalPage(true);
    wizard.addPage(new QWizardPage);

    if (QPushButton *pb = qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton)))
        pb->setAutoDefault(false);
    if (QPushButton *pb = qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton)))
        pb->setAutoDefault(false);

    wizard.show();
    qApp->processEvents();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(wizard.button(QWizard::FinishButton)->isEnabled());

    wizard.next();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.back();
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());

    wizard.setOption(QWizard::NoDefaultButton, true);
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.next();
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.back();
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
    QVERIFY(!qobject_cast<QPushButton *>(wizard.button(QWizard::FinishButton))->isDefault());

    wizard.setOption(QWizard::NoDefaultButton, false);
    QVERIFY(qobject_cast<QPushButton *>(wizard.button(QWizard::NextButton))->isDefault());
}

void tst_QWizard::setOption_NoBackButtonOnStartPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

    wizard.next();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

    wizard.back();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());
}

void tst_QWizard::setOption_NoBackButtonOnLastPage()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.setOption(QWizard::NoBackButtonOnLastPage, true);
        wizard.setOption(QWizard::DisabledBackButtonOnLastPage, i == 0);    // changes nothing
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
        wizard.addPage(new QWizardPage);

        wizard.setStartId(1);
        wizard.show();
        qApp->processEvents();

        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.back();
        qApp->processEvents();
        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.next();
        qApp->processEvents();
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.setOption(QWizard::NoBackButtonOnLastPage, false);
        QVERIFY(wizard.button(QWizard::BackButton)->isVisible());

        wizard.setOption(QWizard::NoBackButtonOnLastPage, true);
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());

        wizard.addPage(new QWizardPage);
        QVERIFY(!wizard.button(QWizard::BackButton)->isVisible());  // this is maybe wrong
    }
}

void tst_QWizard::setOption_DisabledBackButtonOnLastPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, true);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.back();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::BackButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    qApp->processEvents();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, false);
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    wizard.setOption(QWizard::DisabledBackButtonOnLastPage, true);
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.addPage(new QWizardPage);
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());  // this is maybe wrong
}

void tst_QWizard::setOption_HaveNextButtonOnLastPage()
{
    QWizard wizard;
    wizard.setOption(QWizard::HaveNextButtonOnLastPage, false);
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);     // changes nothing (final != last in general)
    wizard.addPage(new QWizardPage);

    wizard.setStartId(1);
    wizard.show();
    qApp->processEvents();

    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, true);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());

    wizard.back();
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, false);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(wizard.button(QWizard::NextButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::NextButton)->isVisible());

    wizard.setOption(QWizard::HaveNextButtonOnLastPage, true);
    QVERIFY(wizard.button(QWizard::NextButton)->isVisible());
    QVERIFY(!wizard.button(QWizard::NextButton)->isEnabled());
}

void tst_QWizard::setOption_HaveFinishButtonOnEarlyPages()
{
    QWizard wizard;
    QVERIFY(!wizard.testOption(QWizard::HaveFinishButtonOnEarlyPages));
    wizard.addPage(new QWizardPage);
    wizard.addPage(new QWizardPage);
    wizard.page(1)->setFinalPage(true);
    wizard.addPage(new QWizardPage);

    wizard.show();
    qApp->processEvents();

    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());

    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.next();
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
    QVERIFY(wizard.button(QWizard::FinishButton)->isVisible());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::FinishButton)->isVisible());
}

void tst_QWizard::setOption_NoCancelButton()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        wizard.setOption(QWizard::NoCancelButton, true);
        wizard.setOption(QWizard::CancelButtonOnLeft, i == 0);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.next();
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButton, false);
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.back();
        QVERIFY(wizard.button(QWizard::CancelButton)->isVisible());

        wizard.setOption(QWizard::NoCancelButton, true);
        QVERIFY(!wizard.button(QWizard::CancelButton)->isVisible());
    }
}

void tst_QWizard::setOption_CancelButtonOnLeft()
{
    for (int i = 0; i < 2; ++i) {
        int sign = (i == 0) ? +1 : -1;

        QWizard wizard;
        wizard.setLayoutDirection(i == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        wizard.setOption(QWizard::NoCancelButton, false);
        wizard.setOption(QWizard::CancelButtonOnLeft, true);
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x()
                < sign * wizard.button(QWizard::BackButton)->x());

        wizard.next();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x()
                < sign * wizard.button(QWizard::BackButton)->x());

        wizard.setOption(QWizard::CancelButtonOnLeft, false);
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x()
                > sign * wizard.button(QWizard::BackButton)->x());

        wizard.back();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::CancelButton)->x()
                > sign * wizard.button(QWizard::BackButton)->x());
    }
}

void tst_QWizard::setOption_HaveHelpButton()
{
    for (int i = 0; i < 2; ++i) {
        QWizard wizard;
        QVERIFY(!wizard.testOption(QWizard::HaveHelpButton));
        wizard.setOption(QWizard::HaveHelpButton, false);
        wizard.setOption(QWizard::HelpButtonOnRight, i == 0);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

        wizard.next();
        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());

        wizard.setOption(QWizard::HaveHelpButton, true);
        QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

        wizard.back();
        QVERIFY(wizard.button(QWizard::HelpButton)->isVisible());

        wizard.setOption(QWizard::HaveHelpButton, false);
        QVERIFY(!wizard.button(QWizard::HelpButton)->isVisible());
    }
}

void tst_QWizard::setOption_HelpButtonOnRight()
{
    for (int i = 0; i < 2; ++i) {
        int sign = (i == 0) ? +1 : -1;

        QWizard wizard;
        wizard.setLayoutDirection(i == 0 ? Qt::LeftToRight : Qt::RightToLeft);
        wizard.setOption(QWizard::HaveHelpButton, true);
        wizard.setOption(QWizard::HelpButtonOnRight, false);
        wizard.setOption(QWizard::NoBackButtonOnStartPage, false);
        wizard.addPage(new QWizardPage);
        wizard.addPage(new QWizardPage);
        wizard.show();
        qApp->processEvents();

        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x()
                < sign * wizard.button(QWizard::BackButton)->x());

        wizard.next();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x()
                < sign * wizard.button(QWizard::BackButton)->x());

        wizard.setOption(QWizard::HelpButtonOnRight, true);
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x()
                > sign * wizard.button(QWizard::BackButton)->x());

        wizard.back();
        qApp->processEvents();
        QVERIFY(sign * wizard.button(QWizard::HelpButton)->x()
                > sign * wizard.button(QWizard::BackButton)->x());
    }
}

void tst_QWizard::setOption_HaveCustomButtonX()
{
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                QWizard wizard;
                wizard.setLayoutDirection(Qt::LeftToRight);
                wizard.addPage(new QWizardPage);
                wizard.addPage(new QWizardPage);
                wizard.show();

                wizard.setButtonText(QWizard::CustomButton1, "Foo");
                wizard.setButton(QWizard::CustomButton2, new QCheckBox("Bar"));
                wizard.button(QWizard::CustomButton3)->setText("Baz");

                wizard.setOption(QWizard::HaveCustomButton1, i == 0);
                wizard.setOption(QWizard::HaveCustomButton2, j == 0);
                wizard.setOption(QWizard::HaveCustomButton3, k == 0);

                QVERIFY(wizard.button(QWizard::CustomButton1)->isHidden() == (i != 0));
                QVERIFY(wizard.button(QWizard::CustomButton2)->isHidden() == (j != 0));
                QVERIFY(wizard.button(QWizard::CustomButton3)->isHidden() == (k != 0));

                if (i + j + k == 0) {
                    qApp->processEvents();
                    QVERIFY(wizard.button(QWizard::CustomButton1)->x()
                            < wizard.button(QWizard::CustomButton2)->x());
                    QVERIFY(wizard.button(QWizard::CustomButton2)->x()
                            < wizard.button(QWizard::CustomButton3)->x());
                }
            }
        }
    }
}

class Operation
{
public:
    virtual void apply(QWizard *) const = 0;
    virtual QString describe() const = 0;
protected:
    virtual ~Operation() {}
};

class SetPage : public Operation
{
    void apply(QWizard *wizard) const
    {
        wizard->restart();
        for (int j = 0; j < page; ++j)
            wizard->next();
    }
    QString describe() const { return QString("set page %1").arg(page); }
    const int page;
public:
    SetPage(int page) : page(page) {}
};

class SetStyle : public Operation
{
    void apply(QWizard *wizard) const { wizard->setWizardStyle(style); }
    QString describe() const { return QString("set style %1").arg(style); }
    const QWizard::WizardStyle style;
public:
    SetStyle(QWizard::WizardStyle style) : style(style) {}
};

class SetOption : public Operation
{
    void apply(QWizard *wizard) const { wizard->setOption(option, on); }
    QString describe() const;
    const QWizard::WizardOption option;
    const bool on;
public:
    SetOption(QWizard::WizardOption option, bool on) : option(option), on(on) {}
};

class OptionInfo
{
    OptionInfo()
    {
        tags[QWizard::IndependentPages]             = "0/IPP";
        tags[QWizard::IgnoreSubTitles]              = "1/IST";
        tags[QWizard::ExtendedWatermarkPixmap]      = "2/EWP";
        tags[QWizard::NoDefaultButton]              = "3/NDB";
        tags[QWizard::NoBackButtonOnStartPage]      = "4/BSP";
        tags[QWizard::NoBackButtonOnLastPage]       = "5/BLP";
        tags[QWizard::DisabledBackButtonOnLastPage] = "6/DLP";
        tags[QWizard::HaveNextButtonOnLastPage]     = "7/NLP";
        tags[QWizard::HaveFinishButtonOnEarlyPages] = "8/FEP";
        tags[QWizard::NoCancelButton]               = "9/NCB";
        tags[QWizard::CancelButtonOnLeft]           = "10/CBL";
        tags[QWizard::HaveHelpButton]               = "11/HHB";
        tags[QWizard::HelpButtonOnRight]            = "12/HBR";
        tags[QWizard::HaveCustomButton1]            = "13/CB1";
        tags[QWizard::HaveCustomButton2]            = "14/CB2";
        tags[QWizard::HaveCustomButton3]            = "15/CB3";

        for (int i = 0; i < 2; ++i) {
            QMap<QWizard::WizardOption, Operation *> operations_;
            foreach (QWizard::WizardOption option, tags.keys())
                operations_[option] = new SetOption(option, i == 1);
            operations << operations_;
        }
    }
    OptionInfo(OptionInfo const&);
    OptionInfo& operator=(OptionInfo const&);
    QMap<QWizard::WizardOption, QString> tags;
    QList<QMap<QWizard::WizardOption, Operation *> > operations;
public:
    static OptionInfo &instance()
    {
        static OptionInfo optionInfo;
        return optionInfo;
    }

    QString tag(QWizard::WizardOption option) const { return tags.value(option); }
    Operation * operation(QWizard::WizardOption option, bool on) const
    { return operations.at(on).value(option); }
    QList<QWizard::WizardOption> options() const { return tags.keys(); }
};

QString SetOption::describe() const
{ 
    return QString("set opt %1 %2").arg(OptionInfo::instance().tag(option)).arg(on);
}

Q_DECLARE_METATYPE(Operation *)
Q_DECLARE_METATYPE(SetPage *)
Q_DECLARE_METATYPE(SetStyle *)
Q_DECLARE_METATYPE(SetOption *)
Q_DECLARE_METATYPE(QList<Operation *>)

class TestGroup
{
public:
    enum Type {Equality, NonEquality};

    TestGroup(const QString &name = QString("no name"), Type type = Equality)
        : name(name), type(type), nRows_(0) {}

    void reset(const QString &name, Type type = Equality)
    {
        this->name = name;
        this->type = type;
        combinations.clear();
    }

    QList<Operation *> &add()
    { combinations << new QList<Operation *>; return *(combinations.last()); }

    void createTestRows()
    {
        for (int i = 0; i < combinations.count(); ++i) {
            QTest::newRow((name + QString(", row %1").arg(i)).toLatin1().data())
                << (i == 0) << (type == Equality) << *(combinations.at(i));
            ++nRows_;
        }
    }

    int nRows() const { return nRows_; }

private:
    QString name;
    Type type;
    int nRows_;
    QList<QList<Operation *> *> combinations;
};

class IntroPage : public QWizardPage
{
    Q_OBJECT
public:
    IntroPage()
    {
        setTitle(tr("Intro"));
        setSubTitle(tr("Intro Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Intro Label")));
        setLayout(layout);
    }
};

class MiddlePage : public QWizardPage
{
    Q_OBJECT
public:
    MiddlePage()
    {
        setTitle(tr("Middle"));
        setSubTitle(tr("Middle Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Middle Label")));
        setLayout(layout);
    }
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT
public:
    ConclusionPage()
    {
        setTitle(tr("Conclusion"));
        setSubTitle(tr("Conclusion Subtitle"));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel(tr("Conclusion Label")));
        setLayout(layout);
    }
};

class TestWizard : public QWizard
{
    Q_OBJECT
    QList<int> pageIds;
    QString opsDescr;
public:
    TestWizard()
    {
        setPixmap(QWizard::BannerPixmap, QPixmap(":/images/banner.png"));
        setPixmap(QWizard::BackgroundPixmap, QPixmap(":/images/background.png"));
        setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));
        setPixmap(QWizard::LogoPixmap, QPixmap(":/images/logo.png"));
        setButtonText(QWizard::CustomButton1, "custom 1");
        setButtonText(QWizard::CustomButton2, "custom 2");
        setButtonText(QWizard::CustomButton3, "custom 3");
        pageIds << addPage(new IntroPage);
        pageIds << addPage(new MiddlePage);
        pageIds << addPage(new ConclusionPage);

        // Disable antialiased font rendering since this may sometimes result in tiny
        // and (apparent) non-deterministic pixel variations between images expected to be
        // identical. This may only be a problem on X11.
        QFont f = font();
        f.setStyleStrategy(QFont::NoAntialias);
        setFont(f);

        // ### Required to prevent a bug(?) in QWizard:
//        setFixedSize(800, 600);
    }

    ~TestWizard()
    {
        foreach (int id, pageIds)
            delete page(id);
    }

    void applyOperations(const QList<Operation *> &operations)
    {
        foreach (Operation * op, operations) {
            if (op) {
                op->apply(this);
                opsDescr += QString("(%1) ").arg(op->describe());
            }
        }
    }

    QImage createImage() const
    {
        return QPixmap::grabWidget(const_cast<TestWizard *>(this))
            .toImage().convertToFormat(QImage::Format_ARGB32);
    }

    QString operationsDescription() const { return opsDescr; }
};

class CombinationsTestData
{
    TestGroup testGroup;
    QList<Operation *> pageOps;
    QList<Operation *> styleOps;
    QMap<bool, QList<Operation *> *> setAllOptions;
public:
    CombinationsTestData()
    {
        QTest::addColumn<bool>("ref");
        QTest::addColumn<bool>("testEquality");
        QTest::addColumn<QList<Operation *> >("operations");
        pageOps << new SetPage(0) << new SetPage(1) << new SetPage(2);
        styleOps << new SetStyle(QWizard::ClassicStyle) << new SetStyle(QWizard::ModernStyle)
                 << new SetStyle(QWizard::MacStyle);
#define SETPAGE(page) pageOps.at(page)
#define SETSTYLE(style) styleOps.at(style)
#define OPT(option, on) OptionInfo::instance().operation(option, on)
#define CLROPT(option) OPT(option, false)
#define SETOPT(option) OPT(option, true)
        setAllOptions[false] = new QList<Operation *>;
        setAllOptions[true]  = new QList<Operation *>;
        foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
            *setAllOptions.value(false) << CLROPT(option);
            *setAllOptions.value(true) << SETOPT(option);
        }
#define CLRALLOPTS *setAllOptions.value(false)
#define SETALLOPTS *setAllOptions.value(true)
    }

    int nRows() const { return testGroup.nRows(); }

    // Creates "all" possible test rows. (WARNING: This typically makes the test take too long!)
    void createAllTestRows()
    {
        testGroup.reset("testAll 1.1");
        testGroup.add(); // i.e. no operations applied!
        testGroup.add() << SETPAGE(0);
        testGroup.add() << SETSTYLE(0);
        testGroup.add() << SETPAGE(0) << SETSTYLE(0);
        testGroup.add() << SETSTYLE(0) << SETPAGE(0);
        testGroup.createTestRows();

        testGroup.reset("testAll 2.1");
        testGroup.add();
        testGroup.add() << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.2");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << SETALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.3");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << CLRALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.4");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << SETALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.5");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << CLRALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.6");
        testGroup.add() << SETALLOPTS;
        testGroup.add() << SETALLOPTS << CLRALLOPTS << SETALLOPTS;
        testGroup.createTestRows();

        testGroup.reset("testAll 2.7");
        testGroup.add() << CLRALLOPTS;
        testGroup.add() << CLRALLOPTS << SETALLOPTS << CLRALLOPTS;
        testGroup.createTestRows();

        for (int i = 0; i < 2; ++i) {
            QList<Operation *> setOptions = *setAllOptions.value(i == 1);

            testGroup.reset("testAll 3.1");
            testGroup.add() << setOptions;
            testGroup.add() << SETPAGE(0) << setOptions;
            testGroup.add() << setOptions << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << setOptions;
            testGroup.add() << setOptions << SETSTYLE(0);
            testGroup.add() << setOptions << SETPAGE(0) << SETSTYLE(0);
            testGroup.add() << SETPAGE(0) << setOptions << SETSTYLE(0);
            testGroup.add() << SETPAGE(0) << SETSTYLE(0) << setOptions;
            testGroup.add() << setOptions << SETSTYLE(0) << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << setOptions << SETPAGE(0);
            testGroup.add() << SETSTYLE(0) << SETPAGE(0) << setOptions;
            testGroup.createTestRows();
        }

        foreach (Operation *pageOp, pageOps) {
            testGroup.reset("testAll 4.1");
            testGroup.add() << pageOp;
            testGroup.add() << pageOp << pageOp;
            testGroup.createTestRows();

            for (int i = 0; i < 2; ++i) {
                QList<Operation *> optionOps = *setAllOptions.value(i == 1);
                testGroup.reset("testAll 4.2");
                testGroup.add() << optionOps << pageOp;
                testGroup.add() << pageOp << optionOps;
                testGroup.createTestRows();

                foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                    Operation *optionOp = OPT(option, i == 1);
                    testGroup.reset("testAll 4.3");
                    testGroup.add() << optionOp << pageOp;
                    testGroup.add() << pageOp << optionOp;
                    testGroup.createTestRows();
                }
            }
        }

        foreach (Operation *styleOp, styleOps) {
            testGroup.reset("testAll 5.1");
            testGroup.add() << styleOp;
            testGroup.add() << styleOp << styleOp;
            testGroup.createTestRows();

            for (int i = 0; i < 2; ++i) {
                QList<Operation *> optionOps = *setAllOptions.value(i == 1);
                testGroup.reset("testAll 5.2");
                testGroup.add() << optionOps << styleOp;
                testGroup.add() << styleOp << optionOps;
                testGroup.createTestRows();

                foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                    Operation *optionOp = OPT(option, i == 1);
                    testGroup.reset("testAll 5.3");
                    testGroup.add() << optionOp << styleOp;
                    testGroup.add() << styleOp << optionOp;
                    testGroup.createTestRows();
                }
            }
        }

        foreach (Operation *pageOp, pageOps) {
            foreach (Operation *styleOp, styleOps) {

                testGroup.reset("testAll 6.1");
                testGroup.add() << pageOp;
                testGroup.add() << pageOp << pageOp;
                testGroup.createTestRows();

                testGroup.reset("testAll 6.2");
                testGroup.add() << styleOp;
                testGroup.add() << styleOp << styleOp;
                testGroup.createTestRows();

                testGroup.reset("testAll 6.3");
                testGroup.add() << pageOp << styleOp;
                testGroup.add() << styleOp << pageOp;
                testGroup.createTestRows();

                for (int i = 0; i < 2; ++i) {
                    QList<Operation *> optionOps = *setAllOptions.value(i == 1);
                    testGroup.reset("testAll 6.4");
                    testGroup.add() << optionOps << pageOp << styleOp;
                    testGroup.add() << pageOp << optionOps << styleOp;
                    testGroup.add() << pageOp << styleOp << optionOps;
                    testGroup.add() << optionOps << styleOp << pageOp;
                    testGroup.add() << styleOp << optionOps << pageOp;
                    testGroup.add() << styleOp << pageOp << optionOps;
                    testGroup.createTestRows();

                    foreach (QWizard::WizardOption option, OptionInfo::instance().options()) {
                        Operation *optionOp = OPT(option, i == 1);
                        testGroup.reset("testAll 6.5");
                        testGroup.add() << optionOp << pageOp << styleOp;
                        testGroup.add() << pageOp << optionOp << styleOp;
                        testGroup.add() << pageOp << styleOp << optionOp;
                        testGroup.add() << optionOp << styleOp << pageOp;
                        testGroup.add() << styleOp << optionOp << pageOp;
                        testGroup.add() << styleOp << pageOp << optionOp;
                        testGroup.createTestRows();
                    }
                }
            }
        }

        testGroup.reset("testAll 7.1", TestGroup::NonEquality);
        testGroup.add() << SETPAGE(0);
        testGroup.add() << SETPAGE(1);
        testGroup.add() << SETPAGE(2);
        testGroup.createTestRows();

        testGroup.reset("testAll 7.2", TestGroup::NonEquality);
        testGroup.add() << SETSTYLE(0);
        testGroup.add() << SETSTYLE(1);
        testGroup.add() << SETSTYLE(2);
        testGroup.createTestRows();

        // more to follow ...
    }

    // Creates a "small" number of interesting test rows.
    void createTestRows1()
    {
        testGroup.reset("test1 1");
        testGroup.add() << SETPAGE(0) << SETOPT(QWizard::HaveCustomButton3);
        testGroup.add() << SETOPT(QWizard::HaveCustomButton3);
        testGroup.createTestRows();

        testGroup.reset("test1 2");
        testGroup.add() << SETOPT(QWizard::HaveFinishButtonOnEarlyPages) << SETPAGE(0);
        testGroup.add() << SETPAGE(0) << SETOPT(QWizard::HaveFinishButtonOnEarlyPages);
        testGroup.createTestRows();

        testGroup.reset("test1 3");
        testGroup.add() << SETPAGE(2) << SETOPT(QWizard::HaveNextButtonOnLastPage);
        testGroup.add() << SETOPT(QWizard::HaveNextButtonOnLastPage) << SETPAGE(2);
        testGroup.createTestRows();
    }
};

void tst_QWizard::combinations_data()
{
    CombinationsTestData combTestData;
//    combTestData.createAllTestRows();
    combTestData.createTestRows1();

    qDebug() << "test rows:" << combTestData.nRows();
}

void tst_QWizard::combinations()
{
    QFETCH(bool, ref);
    QFETCH(bool, testEquality);
    QFETCH(QList<Operation *>, operations);

    TestWizard wizard;
    wizard.applyOperations(operations);
    wizard.show(); // ### Required, but why? Should wizard.createImage() care?

    static QImage refImage;
    static QSize refMinSize;
    static QString refDescr;

    if (ref) {
        refImage = wizard.createImage();
        refMinSize = wizard.minimumSize();
        refDescr = wizard.operationsDescription();
        return;
    }

    QImage image = wizard.createImage();

    bool minSizeTest = wizard.minimumSize() != refMinSize;
    bool imageTest = image != refImage;
    QLatin1String otor("!=");
    QLatin1String reason("differ");

    if (!testEquality) {
        minSizeTest = false; // the image test is sufficient!
        imageTest = !imageTest;
        otor = QLatin1String("==");
        reason = QLatin1String("are equal");
    }

    if (minSizeTest)
        qDebug() << "minimum sizes" << reason.latin1() << ";" << wizard.minimumSize()
                 << otor.latin1() << refMinSize;

    if (imageTest)
        qDebug() << "images" << reason.latin1();

    if (minSizeTest || imageTest) {
        qDebug() << "\t      row 0 operations:" << refDescr.toLatin1();
        qDebug() << "\tcurrent row operations:" << wizard.operationsDescription().toLatin1();
        QVERIFY(false);
    }
}

class WizardPage : public QWizardPage
{
    Q_OBJECT
    bool shown_;
    void showEvent(QShowEvent *) { shown_ = true; }
    void hideEvent(QHideEvent *) { shown_ = false; }
public:
    WizardPage() : shown_(false) {}
    bool shown() const { return shown_; }
};

class WizardPages
{
    QList<WizardPage *> pages;
public:
    void add(WizardPage *page) { pages << page; }
    QList<WizardPage *> all() const { return pages; }
    QList<WizardPage *> shown() const
    {
        QList<WizardPage *> result;
        foreach (WizardPage *page, pages)
            if (page->shown())
                result << page;
        return result;
    }
};

void tst_QWizard::showCurrentPageOnly()
{
    QWizard wizard;
    WizardPages pages;
    for (int i = 0; i < 5; ++i) {
        pages.add(new WizardPage);
        wizard.addPage(pages.all().last());
    }

    wizard.show();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().first());

    const int steps = 2;
    for (int i = 0; i < steps; ++i)
        wizard.next();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().at(steps));

    wizard.restart();

    QCOMPARE(pages.shown().count(), 1);
    QCOMPARE(pages.shown().first(), pages.all().first());
}

void tst_QWizard::setButtonText()
{
    QWizard wizard;
    wizard.setWizardStyle(QWizard::ClassicStyle);
    QWizardPage* page1 = new QWizardPage;
    QWizardPage* page2 = new QWizardPage;
    wizard.addPage(page1);
    wizard.addPage(page2);

    wizard.show();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page2->buttonText(QWizard::NextButton).contains("Next"));

    page2->setButtonText(QWizard::NextButton, "Page2");
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.next();
    qApp->processEvents();
    QCOMPARE(wizard.button(QWizard::NextButton)->text(), QString("Page2"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.back();
    qApp->processEvents();
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Next"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Next"));
    QVERIFY(page1->buttonText(QWizard::NextButton).contains("Next"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.setButtonText(QWizard::NextButton, "Wizard");
    QVERIFY(wizard.button(QWizard::NextButton)->text().contains("Wizard"));
    QCOMPARE(wizard.buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page1->buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));

    wizard.next();
    qApp->processEvents();
    QCOMPARE(wizard.button(QWizard::NextButton)->text(), QString("Page2"));
    QVERIFY(wizard.buttonText(QWizard::NextButton).contains("Wizard"));
    QCOMPARE(page1->buttonText(QWizard::NextButton), QString("Wizard"));
    QCOMPARE(page2->buttonText(QWizard::NextButton), QString("Page2"));
}

void tst_QWizard::setCommitPage()
{
    QWizard wizard;
    QWizardPage* page1 = new QWizardPage;
    QWizardPage* page2 = new QWizardPage;
    wizard.addPage(page1);
    wizard.addPage(page2);
    wizard.show();
    qApp->processEvents();

    QVERIFY(!page1->isCommitPage());
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    page1->setCommitPage(true);
    QVERIFY(page1->isCommitPage());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    page1->setCommitPage(false);
    QVERIFY(!page1->isCommitPage());

    wizard.back();
    QVERIFY(!wizard.button(QWizard::BackButton)->isEnabled());

    wizard.next();
    QVERIFY(wizard.button(QWizard::BackButton)->isEnabled());

    // ### test relabeling of the Cancel button to "Close" once this is implemented
}

void tst_QWizard::setWizardStyle()
{
    QWizard wizard;
    wizard.addPage(new QWizardPage);
    wizard.show();
    qApp->processEvents();

    // defaults
    const bool styleHintMatch =
        wizard.wizardStyle() ==
        QWizard::WizardStyle(wizard.style()->styleHint(QStyle::SH_WizardStyle, 0, &wizard));
#if !defined(QT_NO_STYLE_WINDOWSVISTA)
    QVERIFY(styleHintMatch || wizard.wizardStyle() == QWizard::AeroStyle);
#else
    QVERIFY(styleHintMatch);
#endif

    // set/get consistency
    for (int wstyle = 0; wstyle < QWizard::NStyles; ++wstyle) {
        wizard.setWizardStyle((QWizard::WizardStyle)wstyle);
        QCOMPARE((int)wizard.wizardStyle(), wstyle);
    }
}

QTEST_MAIN(tst_QWizard)
#include "tst_qwizard.moc"
