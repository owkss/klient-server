#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QListWidget;
class QProgressBar;
class QTableWidget;
class QPlainTextEdit;
QT_END_NAMESPACE

enum Type : quint16 { NoType = 0, Message = 3, FileList = 5, File = 7 };

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();
    void about();

private slots:
    void customMenuRequested(const QPoint pos);
    void on_readyRead();
    void on_connected();
    void displayError(QAbstractSocket::SocketError socketError);

private:
    void createUI();
    void createConnection();
    void breakConnection();
    void deleteFile(const QModelIndex index);
    void showHistory();
    void addEvent(QString event);
    void updateList();
    void requireFile(const QModelIndex &index);
    void openFile(const QModelIndex &index);

    QTcpSocket socket;
    QDataStream in;

    QStringList fileList;
    QString fileName;
    quint16 type = Type::NoType;
    quint32 bytesReceived;
    quint32 totalSize{0};

    // UI
    QMenu *mainMenu = nullptr;
    QAction *connectAct = nullptr;
    QAction *disconnectAct = nullptr;
    QAction *showHistoryAct = nullptr;

    QListWidget *downloadedFiles = nullptr;
    QListWidget *listWidget = nullptr;
    QTableWidget *table = nullptr;
    QPlainTextEdit *history = nullptr;
    QWidget *historyWidget = nullptr;
    QPlainTextEdit *textContent = nullptr;
    QProgressBar *downloadProgress = nullptr;
};

#endif // CLIENT_H
