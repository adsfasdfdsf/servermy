#include "server.h"
#include <QTcpServer>
#include <QTcpSocket>


Server::Server(QObject *parent)
    : QObject{parent}
{
    server_ptr = new QTcpServer;
    connect(server_ptr, &QTcpServer::newConnection,
            this, &Server::onNewConnection);
    if (!server_ptr->listen(QHostAddress::Any, 1234)){
        qDebug() << "Listen failed";
        exit(1);
    }
    qDebug() << "Listen success";
}

void Server::onNewConnection()
{
    auto clsocket = server_ptr->nextPendingConnection();
    clients.push_back(clsocket);
    connect(clsocket, &QTcpSocket::readyRead,
            this, &Server::onNewMessage);
    connect(clsocket, &QTcpSocket::disconnected,
            this, &Server::onDisconnect);
}

void Server::onDisconnect(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    socket_ptr->close();
    clients.removeOne(socket_ptr);
    qDebug() << "Client: ";
}

void Server::onNewMessage(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    while(socket_ptr->bytesAvailable() > 0){
        auto msg = socket_ptr->readAll();
        qDebug() << msg;
        for (const auto& cl: clients){
            cl->write(msg);
        }
    }
}

