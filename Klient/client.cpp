#include "client.h"
#include "connection.h"
#include "fileinfo.h"
#include "datastruct.h"

#include <QTime>
#include <QMenuBar>
#include <QStatusBar>
#include <QGroupBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QProgressBar>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QCryptographicHash>

Client::Client(QWidget *parent)
    : QMainWindow(parent)
{
    createUI();

    connect(&socket, &QIODevice::readyRead, this, &Client::on_readyRead);
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &Client::displayError);

    in.setDevice(&socket);
    in.setVersion(QDataStream::Qt_5_9);

    bytesReceived = sizeof(type) + sizeof(totalSize);

    connect(&socket, &QAbstractSocket::connected, this, &Client::on_connected);
    connect(&socket, &QAbstractSocket::disconnected,
            [=]() { listWidget->clear(); downloadedFiles->clear(); fileList.clear(); });
}

Client::~Client()
{
    delete historyWidget;
}

void
Client::createUI()
{
    statusBar()->show();
    mainMenu = menuBar()->addMenu("Подключение");
    connectAct = mainMenu->addAction("Подключиться", this, &Client::createConnection);
    disconnectAct = mainMenu->addAction("Отключиться", this, &Client::breakConnection);
    mainMenu->addSeparator();
    showHistoryAct = mainMenu->addAction("Показать историю", this, &Client::showHistory);
    mainMenu->addSeparator();
    QAction *exitAct = mainMenu->addAction("Выход", this, &QWidget::close);
    connectAct->setStatusTip("Подключение к выбранному серверу");
    disconnectAct->setStatusTip("Отключиться от сервера");
    showHistoryAct->setStatusTip("Показать лог");
    exitAct->setStatusTip("Выход из программы");

    QMenu *aboutMenu = menuBar()->addMenu("Справка");
    QAction *aboutAct = aboutMenu->addAction("О программе", this, &Client::about);
    QAction *aboutQt = aboutMenu->addAction("О Qt", this, &QApplication::aboutQt);
    aboutAct->setStatusTip("Информация о программе");
    aboutQt->setStatusTip("Информация о фреймворке Qt и лицензиях");

    history = new QPlainTextEdit;
    history->setReadOnly(true);

    QWidget *mainWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(mainWidget);
    listWidget = new QListWidget(mainWidget);

    downloadedFiles = new QListWidget(mainWidget);
    downloadedFiles->setSelectionMode(QAbstractItemView::SingleSelection);
    downloadedFiles->setContextMenuPolicy(Qt::CustomContextMenu);

    QGroupBox *listGroup = new QGroupBox(mainWidget);
    QGridLayout *listGrid = new QGridLayout(listGroup);
    QGroupBox *filesGroup = new QGroupBox(mainWidget);
    QGridLayout *filesGrid = new QGridLayout(filesGroup);

    listGroup->setTitle("Список файлов");
    listGrid->addWidget(listWidget);
    listGroup->setLayout(listGrid);
    listGroup->setMinimumSize(QSize(200, 400));

    filesGroup->setTitle("Сохранённые файлы");
    filesGrid->addWidget(downloadedFiles);
    filesGroup->setLayout(filesGrid);
    filesGroup->setMinimumSize(QSize(200, 400));

    QSplitter *splitter = new QSplitter(mainWidget);
    splitter->addWidget(listGroup);
    splitter->addWidget(filesGroup);
    layout->addWidget(splitter);

    downloadProgress = new QProgressBar(mainWidget);
    downloadProgress->setToolTip("Полоса загрузки");
    listGrid->addWidget(downloadProgress);

    mainWidget->setLayout(layout);
    setCentralWidget(mainWidget);

    connect(listWidget, &QAbstractItemView::doubleClicked, this, &Client::requireFile);
    connect(downloadedFiles, &QAbstractItemView::doubleClicked, this, &Client::openFile);
    connect(downloadedFiles, &QWidget::customContextMenuRequested, this, &Client::customMenuRequested);

    addEvent("Программа запущена");
}

void
Client::showHistory()
{
    historyWidget = new QWidget;
    QGridLayout *historyGrid = new QGridLayout(historyWidget);
    historyGrid->addWidget(history);
    historyWidget->setWindowTitle("История");
    historyWidget->setLayout(historyGrid);
    historyWidget->resize(300, 500);
    historyWidget->show();
}

void
Client::addEvent(QString event)
{
    history->appendPlainText(QTime::currentTime().toString() + ": " + event);
}

void
Client::createConnection()
{   
    ConnectDialog *newConnection = new ConnectDialog;
    newConnection->setWindowTitle("Параметры соединения");

    if (newConnection->exec())
    {
        socket.abort();
        socket.connectToHost(newConnection->currentHost,
                             static_cast<quint16>(newConnection->currentPort));
        addEvent("Установка соединения...");
    }
    else
    {
        delete newConnection;
        return;
    }

    delete newConnection;
}

void
Client::breakConnection()
{
    socket.abort();
    listWidget->clear();
    fileList.clear();
    addEvent("Соединение разорвано");
}

void
Client::on_readyRead()
{
    in.startTransaction();

    in >> type;
    if (type == Type::NoType)
        return;

    if (type == Type::Message)
    {
        QString msg;
        in >> msg;

        if (!in.commitTransaction())
            return;

        addEvent("Сообщение от сервера: " + msg);
        statusBar()->showMessage(msg, 5000);
    }

    if (type == Type::FileList)
    {
        in >> fileList;

        if (!in.commitTransaction())
            return;

        updateList();
        addEvent("Получен новый список файлов");
        statusBar()->showMessage("Список файлов обновлён", 5000);
    }

    if (type == Type::File)
    {
        in >> totalSize;

        // Progress
        downloadProgress->setMaximum(totalSize);
        downloadProgress->setMinimum(0);
        bytesReceived += socket.bytesAvailable();
        downloadProgress->setValue(bytesReceived);

        QByteArray originalHash;
        in >> originalHash;

        QByteArray requestedFile;
        in >> requestedFile;

        if (!in.commitTransaction() || (totalSize != requestedFile.size()))
            return;

        QByteArray currentHash = QCryptographicHash::hash(requestedFile, QCryptographicHash::Sha1);
        if (currentHash != originalHash)
        {
            addEvent("Ошибка: несовпадение хэшей");
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            addEvent("Ошибка: невозможно сохранить файл");
            return;
        }

        qint64 writtenSize = file.write(requestedFile);
        if (writtenSize == requestedFile.size())
        {
            file.close();
            originalHash.clear();
            requestedFile.clear();
            socket.readAll();

            // Progress
            downloadProgress->setValue(totalSize);
            bytesReceived = sizeof(type) + sizeof(totalSize);

            QListWidgetItem *item = new QListWidgetItem(fileName);
            downloadedFiles->addItem(item);

            addEvent("Запрашиваемый файл успешно сохранён");
            statusBar()->showMessage(QString("Файл %1 успешно загружен").arg(fileName), 5000);
        }
    }
}

void
Client::on_connected()
{
    addEvent(QString("Соединение установлено: %1:%2")
             .arg(socket.peerName()).arg(socket.peerPort()));
    statusBar()->showMessage("Соединение установлено успешно!", 5000);
}

void
Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        addEvent("Удалённый сервер закрыл соединение");
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Предупреждение"),
                                 tr("Сервер не найден."));
        addEvent("Сервер не найден");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        addEvent("Ошибка соединения");
        QMessageBox::information(this, tr("Предупреждение"),
                                 tr("Ошибка соединения"));
        break;
    default:
        addEvent(QString("Ошибка: %1").arg(socket.errorString()));
        QMessageBox::information(this, tr("Предупреждение"),
                                 tr("Ошибка: %1.")
                                 .arg(socket.errorString()));
    }
}

void
Client::updateList()
{
    listWidget->clear();
    QListWidgetItem *item = nullptr;
    foreach (QString name, fileList)
    {
        item = new QListWidgetItem(name, listWidget);
        item->setIcon(QIcon(":/file.png"));
    }
}

void
Client::requireFile(const QModelIndex &index)
{
    quint16 request = Type::File;
    fileName = index.data().toString();

    for (int i(0); i < downloadedFiles->count(); ++i)
    {
        QListWidgetItem *item = downloadedFiles->item(i);
        if (item->text() == fileName)
        {
            statusBar()->showMessage("Файл уже существует", 5000);
            return;
        }
    }

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    out << request << fileName;

    socket.write(data);

    downloadProgress->setValue(0);
}

void
Client::openFile(const QModelIndex &index)
{
    QString fname = index.data().toString();
    QString fext = fname.section('.', -1);
    QFile file(fname);
    if (!file.exists())
    {
        QMessageBox::information(this, "Файл", "Запрашиваемый файл был перемещён или удалён");
        statusBar()->showMessage("Невозможно открыть файл", 5000);
        addEvent("Ошибка открытия файла: удалён или перемещён");
        return;
    }

    file.open(QIODevice::ReadOnly);
    QDataStream fout(&file);
    fout.setVersion(QDataStream::Qt_5_9);

    if (fext == "dat")
    {
        QStringList headers;
        headers << "Тип" << "X" << "Y" << "Путь";

        table = new QTableWidget;
        table->setAttribute(Qt::WA_DeleteOnClose);
        table->setWindowTitle("Структурированные данные");
        table->setColumnCount(headers.count());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setMinimumSize(QSize(200, 300));

        while (!fout.atEnd())
        {
            SData sd;
            fout >> sd;
            table->insertRow(table->rowCount());
            QTableWidgetItem *l = new QTableWidgetItem(QChar(sd.literal));
            table->setItem(table->rowCount() - 1, 0, l);
            QTableWidgetItem *x = new QTableWidgetItem(QString::number(sd.x));
            table->setItem(table->rowCount() - 1, 1, x);
            QTableWidgetItem *y = new QTableWidgetItem(QString::number(sd.y));
            table->setItem(table->rowCount() - 1, 2, y);
            QTableWidgetItem *t = new QTableWidgetItem(QString::number(sd.length));
            table->setItem(table->rowCount() - 1, 3, t);
        }
        table->resize(QSize(300, 500));
        table->show();
    } else
    {
        textContent = new QPlainTextEdit;
        textContent->setAttribute(Qt::WA_DeleteOnClose);
        textContent->setWindowTitle("Просмотр текстовых документов");
        textContent->setReadOnly(true);

        QString text = QTextCodec::codecForMib(106)->toUnicode(file.readAll());
        textContent->appendPlainText(text);
        textContent->resize(QSize(500, 500));
        textContent->show();
    }
}

void
Client::customMenuRequested(const QPoint pos)
{
    if (!downloadedFiles->selectionModel()->hasSelection())
        return;

    QModelIndex index = downloadedFiles->indexAt(pos);

    QMenu *contextMenu = new QMenu;
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *deleteFileAct = contextMenu->addAction(QIcon(":/delete.png"), "Удалить файл");

    contextMenu->popup(downloadedFiles->viewport()->mapToGlobal(pos));

    connect(deleteFileAct, &QAction::triggered,
            [=]() { deleteFile(index); });
}

void
Client::deleteFile(const QModelIndex index)
{
    QString fname = index.data().toString();
    QFile file(fname);

    if (!file.exists())
    {
        QMessageBox::information(this, "Удаление файла", "Файл не существует");
        addEvent("Ошибка при удалении файла: файл не существует");
        statusBar()->showMessage("Нет такого файла");

        QListWidgetItem *item = downloadedFiles->takeItem(index.row());
        delete item;

        return;
    }

    if (!file.remove())
    {
        QMessageBox::information(this, "Удаление файла", "Файл недоступен");
        addEvent("Невозможно удалить файл");
        statusBar()->showMessage("Файл недоступен");

        return;
    }

    QListWidgetItem *item = downloadedFiles->takeItem(index.row());
    delete item;

    addEvent(QString("Удалено: %1").arg(fname));
    statusBar()->showMessage("Файл успешно удалён", 5000);
}

void
Client::about()
{
    static const QString about =
    tr("<p align=\"center\"><b>Программа для просмотра текстовых файлов"
       " и файлов заданной структуры</b></p>");

    QMessageBox::about(this, tr("О программе"), about);
}
