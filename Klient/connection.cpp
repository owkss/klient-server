#include "connection.h"

#include <QLabel>
#include <QHostInfo>
#include <QGridLayout>
#include <QHostAddress>
#include <QNetworkInterface>

ConnectDialog::ConnectDialog(QWidget *parent)
{
    Q_UNUSED(parent)
    hosts = new QComboBox(this);
    port = new QLineEdit(this);
    QLabel *hostlbl = new QLabel("Адрес:", this);
    QLabel *portlbl = new QLabel("Порт:", this);
    QGridLayout *grid = new QGridLayout(this);
    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    acceptButton = new QPushButton("Ok", this);
    rejectButton = new QPushButton("Отмена", this);

    buttons->addButton(acceptButton, QDialogButtonBox::ActionRole);
    buttons->addButton(rejectButton, QDialogButtonBox::RejectRole);

    hostlbl->setBuddy(hosts);
    portlbl->setBuddy(port);

    grid->addWidget(hostlbl, 0, 0, 1, 1);
    grid->addWidget(hosts, 0, 1, 1, 1);
    grid->addWidget(portlbl, 1, 0, 1, 1);
    grid->addWidget(port, 1, 1, 1, 1);
    grid->addWidget(buttons, 2, 1, 1, 1);

    setLayout(grid);

    hosts->setEditable(true);
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hosts->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hosts->addItem(name + QChar('.') + domain);
    }
    if (name != QLatin1String("localhost"))
        hosts->addItem(QString("localhost"));
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hosts->addItem(ipAddressesList.at(i).toString());
    }
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hosts->addItem(ipAddressesList.at(i).toString());
    }

    port->setValidator(new QIntValidator(1, 65535, this));

    connect(acceptButton, &QAbstractButton::clicked, this, &ConnectDialog::AcceptParameters);
    connect(rejectButton, &QAbstractButton::clicked, this, &QWidget::close);
}

ConnectDialog::~ConnectDialog()
{}

void
ConnectDialog::AcceptParameters()
{
    currentHost = hosts->currentText();
    currentPort = port->text().toInt();
    this->accept();
}
