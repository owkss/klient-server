#ifndef CONNECTION_H
#define CONNECTION_H

#include <QDialog>
#include <QtWidgets>

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    ConnectDialog(QWidget *parent = nullptr);
    ~ConnectDialog();

    QString currentHost;
    int currentPort{0};

private:
    void AcceptParameters();

    QComboBox *hosts = nullptr;
    QLineEdit *port = nullptr;
    QPushButton *acceptButton = nullptr;
    QPushButton *rejectButton = nullptr;
};

#endif // CONNECTION_H
