#include "server.h"
#include "datastruct.h"

#include <QDir>
#include <QLabel>
#include <QHostInfo>
#include <QLineEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkInterface>
#include <QCryptographicHash>

enum Type : quint16 { NoType = 0, Message = 3, FileList = 5, File = 7 };

Server::Server(QWidget *parent)
    : QWidget(parent)
{
    createUI();
    setRootDirectory();
}

Server::~Server()
{}

void
Server::createUI()
{
    QLabel *addresslbl = new QLabel("Адрес:", this);
    QLabel *portlbl = new QLabel("Порт:", this);

    QGridLayout *grid = new QGridLayout(this);

    QGroupBox *listgroup = new QGroupBox(this);
    QGridLayout *listgrid = new QGridLayout(listgroup);

    server = new QTcpServer(this);
    listWidget = new QListWidget(this);
    startButton = new QPushButton("Старт", this);
    selectFolder = new QPushButton("Выбор директории", this);
    edit = new QLineEdit("12345", this);
    comboBox = new QComboBox(this);

    comboBox->setEditable(true);
    comboBox->setMinimumWidth(150);
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        comboBox->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            comboBox->addItem(name + QChar('.') + domain);
    }
    if (name != QLatin1String("localhost"))
        comboBox->addItem(QString("localhost"));
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            comboBox->addItem(ipAddressesList.at(i).toString());
    }
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            comboBox->addItem(ipAddressesList.at(i).toString());
    }

    edit->setMaximumWidth(150);

    listgroup->setLayout(listgrid);
    listgroup->setTitle("Содержимое директории");
    listgrid->addWidget(listWidget);

    // Layouts
    QHBoxLayout *addressLayout = new QHBoxLayout(this);
    addressLayout->addWidget(addresslbl);
    addressLayout->addWidget(comboBox);

    QHBoxLayout *portLayout = new QHBoxLayout(this);
    portLayout->addWidget(portlbl);
    portLayout->addWidget(edit);

    countlbl = new QLabel(QString("Клиенты: %1").arg(clientList.count()), this);

    QVBoxLayout *vbl = new QVBoxLayout(this);
    vbl->addLayout(addressLayout);
    vbl->addLayout(portLayout);
    vbl->addWidget(startButton);
    vbl->addWidget(selectFolder);
    vbl->addWidget(countlbl);

    QWidget *control = new QWidget(this);
    control->setLayout(vbl);

    grid->addWidget(control);
    grid->addWidget(listgroup);

    setLayout(grid);

    edit->setValidator(new QIntValidator(1, 65535, this));

    connect(startButton, &QAbstractButton::clicked, this, &Server::startListening);
    connect(server, &QTcpServer::newConnection, this, &Server::on_newConnection);
    connect(selectFolder, &QAbstractButton::clicked, this, &Server::setRootDirectory);
}

void
Server::startListening()
{
    if (!server->isListening())
    {
        QHostAddress hostaddress(comboBox->currentText());
        if (!server->listen(hostaddress, static_cast<quint16>(edit->text().toInt())))
        {
            QMessageBox::critical(this, tr("Ошибка"),
                                  tr("Ошибка запуска сервера: %1.")
                                  .arg(server->errorString()));
            return;
        }
        startButton->setText("Стоп");
        qDebug() << "Started...";
    }
    else
    {
        foreach (QTcpSocket *client, clientList)
        {
            client->disconnectFromHost();
            sendMessage(client, "Сервер недоступен для новых подключений");
        }
        server->close();
        startButton->setText("Старт");
        qDebug() << "Stopped...";
    }
}

void
Server::setRootDirectory()
{
    folder = QFileDialog::getExistingDirectory(this, "Выбор корневой директории", folder);
    QDir path(folder);
    fileList = path.entryList(QDir::Files);

    listWidget->clear();
    QListWidgetItem *item = nullptr;
    foreach (QString name, fileList)
    {
        item = new QListWidgetItem(name, listWidget);
        item->setIcon(QIcon(":/file.png"));
    }

    if (server->isListening())
    {
        foreach (QTcpSocket *client, clientList)
        {
            sendFileList(client);
        }
    }
}

void
Server::sendMessage(QTcpSocket *client, QString msg)
{
    quint16 r_type = Type::Message;
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    out << r_type << msg;

    client->write(data);
    client->flush();
}

void
Server::sendFileList(QTcpSocket *client)
{
    quint16 r_type = Type::FileList;
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    out << r_type << fileList;

    client->write(data);
    client->flush();
}

void
Server::sendFile(QTcpSocket *client, QString fileName)
{
    quint16 r_type = Type::File;
    QFile file(folder + '/' + fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Ошибка", "Невозможно открыть файл");
        return;
    }

    QByteArray buffer(file.readAll());
    if (buffer.size() == 0 || buffer.size() == -1)
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка при чтении файла");
        return;
    }
    QByteArray fileHash = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    quint32 fsize = static_cast<quint32>(file.size());

    out << r_type << fsize << fileHash << buffer;

    client->write(block);

    file.close();
}

void
Server::on_newConnection()
{
    QTcpSocket *connectionToServer = server->nextPendingConnection();
    clientList.append(connectionToServer);
    countlbl->setText(QString("Клиенты: %1").arg(clientList.count()));

    sendFileList(connectionToServer);

    connect(connectionToServer, &QIODevice::readyRead, this, &Server::on_readyRead);
    connect(connectionToServer, &QAbstractSocket::disconnected, this, &Server::on_disconnected);
}

void
Server::on_readyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    QDataStream in(client);
    in.setVersion(QDataStream::Qt_5_9);

    in.startTransaction();

    in >> type;
    if (type == Type::FileList)
        sendFileList(client);
    else if (type == Type::File)
    {
        QString fileName;
        in >> fileName;
        sendFile(client, fileName);
    }

    if (!in.commitTransaction())
        return;
}

void
Server::on_disconnected()
{
    QTcpSocket *disconnectedClient = qobject_cast<QTcpSocket*>(sender());
    disconnectedClient->deleteLater();

    clientList.removeOne(disconnectedClient);
    countlbl->setText(QString("Клиенты: %1").arg(clientList.count()));
    disconnect(disconnectedClient, &QIODevice::readyRead, nullptr, nullptr);
}
