#ifndef SERVER_H
#define SERVER_H

#include <QObject>
class QTcpServer;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
private:
    QTcpServer* server_ptr;
    QVector<class QTcpSocket*> clients;
private slots:
    void onNewConnection();
    void onDisconnect();
    void onNewMessage();
};

#endif // SERVER_H
