#include <QtGui>
#include <QtNetwork>

#include "ftpwindow.h"

FtpWindow::FtpWindow(QWidget *parent)
    : QDialog(parent)
{
    ftpServerLabel = new QLabel(tr("Ftp &server:"), this);
    ftpServerLineEdit = new QLineEdit("ftp.trolltech.com", this);
    ftpServerLabel->setBuddy(ftpServerLineEdit);

    statusLabel = new QLabel(tr("Please enter the name of an FTP server."),
                             this);

    fileList = new QListWidget(this);

    connectButton = new QPushButton(tr("Connect"), this);

    downloadButton = new QPushButton(tr("Download"), this);
    downloadButton->setEnabled(false);
    downloadButton->setDefault(true);

    cdToParentButton = new QPushButton(this);
    cdToParentButton->setIcon(QPixmap(":/images/cdtoparent.png"));
    cdToParentButton->setEnabled(false);

    quitButton = new QPushButton(tr("Quit"), this);

    ftp = new QFtp(this);

    progressDialog = new QProgressDialog(this);

    connect(ftpServerLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableConnectButton()));
    connect(fileList, SIGNAL(doubleClicked(QListWidgetItem *, Qt::ButtonState)),
            this, SLOT(processItem(QListWidgetItem *)));
    connect(fileList, SIGNAL(returnPressed(QListWidgetItem *)),
            this, SLOT(processItem(QListWidgetItem *)));
    connect(fileList, SIGNAL(selectionChanged()),
            this, SLOT(enableDownloadButton()));
    connect(ftp, SIGNAL(commandFinished(int, bool)),
            this, SLOT(ftpCommandFinished(int, bool)));
    connect(ftp, SIGNAL(listInfo(const QUrlInfo &)),
            this, SLOT(addToList(const QUrlInfo &)));
    connect(ftp, SIGNAL(dataTransferProgress(Q_LONGLONG, Q_LONGLONG)),
            this, SLOT(updateDataTransferProgress(Q_LONGLONG, Q_LONGLONG)));
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectToFtpServer()));
    connect(cdToParentButton, SIGNAL(clicked()), this, SLOT(cdToParent()));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(ftpServerLabel);
    topLayout->addWidget(ftpServerLineEdit);
    topLayout->addWidget(cdToParentButton);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(downloadButton);
    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(quitButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);

    setWindowTitle(tr("FTP"));
}

void FtpWindow::connectToFtpServer()
{
    ftp->connectToHost(ftpServerLineEdit->text());
    ftp->list();
    statusLabel->setText(tr("Connecting to FTP server %1...")
                         .arg(ftpServerLineEdit->text()));
}

void FtpWindow::downloadFile()
{
    QString fileName = fileList->currentItem()->text();

    if (QFile::exists(fileName)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("There already exists a file called %1 in "
                                    "the current directory.")
                                 .arg(fileName));
        return;
    }

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        return;
    }

    ftp->get(fileList->currentItem()->text(), file);

    progressDialog->setLabelText(tr("Downloading %1...").arg(fileName));
    progressDialog->show();
    downloadButton->setEnabled(false);
}

void FtpWindow::cancelDownload()
{
    ftp->abort();
}

void FtpWindow::ftpCommandFinished(int /* commandId */, bool error)
{
    if (ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error) {
            QMessageBox::information(this, tr("FTP"),
                                     tr("Unable to connect to the FTP server "
                                        "at %1. Please check that the host "
                                        "name is correct.")
                                     .arg(ftpServerLineEdit->text()));
            return;
        }

        statusLabel->setText(tr("Connected to %1.")
                             .arg(ftpServerLineEdit->text()));
        fileList->setFocus();
        connectButton->setEnabled(false);
        return;
    }

    if (ftp->currentCommand() == QFtp::Get) {
        if (error) {
            statusLabel->setText(tr("Canceled download of %1.")
                                 .arg(file->fileName()));
            file->close();
            file->remove();
            delete file;
            enableDownloadButton();
            return;
        }

        statusLabel->setText(tr("Downloaded %1 to current directory.")
                             .arg(file->fileName()));
        file->close();
        delete file;
    }

    if (ftp->currentCommand() == QFtp::List) {
        if (isDirectory.isEmpty()) {
            fileList->appendItem(tr("<empty>"));
            fileList->setEnabled(false);
        }
    }
}

void FtpWindow::addToList(const QUrlInfo &urlInfo)
{
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(urlInfo.name());
    QPixmap pixmap(urlInfo.isDir() ? ":/images/dir.png" : ":/images/file.png");
    item->setIcon(pixmap);

    isDirectory[urlInfo.name()] = urlInfo.isDir();
    fileList->appendItem(item);
    if (!fileList->currentItem()) {
        fileList->setCurrentItem(fileList->item(0));
        fileList->setEnabled(true);
    }
}

void FtpWindow::processItem(QListWidgetItem *item)
{
    QString name = item->text();
    if (isDirectory.value(name)) {
        fileList->clear();
        isDirectory.clear();
        currentPath += "/" + name;
        ftp->cd(name);
        ftp->list();
        cdToParentButton->setEnabled(true);
        return;
    }
}

void FtpWindow::cdToParent()
{
    fileList->clear();
    isDirectory.clear();
    currentPath = currentPath.left(currentPath.lastIndexOf('/'));
    ftp->cd(currentPath);
    ftp->list();

    if (currentPath.isEmpty())
        cdToParentButton->setEnabled(false);
}

void FtpWindow::updateDataTransferProgress(Q_LONGLONG readBytes,
                                           Q_LONGLONG totalBytes)
{
    progressDialog->setTotalSteps(totalBytes);
    progressDialog->setProgress(readBytes);
}

void FtpWindow::enableConnectButton()
{
    connectButton->setEnabled(!ftpServerLineEdit->text().isEmpty());
}

void FtpWindow::enableDownloadButton()
{
    QString currentFile = fileList->currentItem()->text();
    downloadButton->setEnabled(!isDirectory.value(currentFile));
}
