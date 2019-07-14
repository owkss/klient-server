#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QFileSystemModel;
QT_END_NAMESPACE

class Server : public QWidget
{
    Q_OBJECT

public:
    Server(QWidget *parent = nullptr);
    ~Server();

private slots:
    void on_newConnection();
    void on_readyRead();
    void on_disconnected();

private:
    void createUI();
    void startListening();
    void setRootDirectory();
    void sendFileList(QTcpSocket *client);
    void sendFile(QTcpSocket *client, QString fileName);
    void sendMessage(QTcpSocket *client, QString msg);

    QTcpServer *server = nullptr;
    quint16 type{0};

    QComboBox *comboBox = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *selectFolder = nullptr;
    QLineEdit *edit = nullptr;
    QLabel *countlbl;

    QString folder;
    QStringList fileList;
    QList<QTcpSocket*> clientList;
    QListWidget *listWidget = nullptr;
};

#endif // SERVER_H
