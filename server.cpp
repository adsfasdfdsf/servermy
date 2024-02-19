#include "server.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QtSql>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

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
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("chat.db");
    db.open();
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, name STRING, message STRING)")){
        qDebug() << query.lastError().text();
    }
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
        QJsonDocument doc = QJsonDocument::fromJson(msg);
        QJsonObject json = doc.object();
        qDebug() << json["name"].toString() << ": " << json["message"].toString();
        QSqlQuery query;
        query.prepare("INSERT INTO messages (name, message) VALUES (:name, :message)");
        query.bindValue(":name", json["name"].toString());
        query.bindValue(":message", json["message"].toString());
        if (!query.exec()){
            qDebug() << query.lastError().text();
        }
        for (const auto& cl: clients){
            cl->write(msg);
        }
    }
}

