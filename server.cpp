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
    qDebug() << "new connection";
    QString clname = clsocket->readAll();
    clients[clsocket] =  clname;
    connect(clsocket, &QTcpSocket::readyRead,
            this, &Server::onNewMessage);
    connect(clsocket, &QTcpSocket::disconnected,
            this, &Server::onDisconnect);

    QSqlQuery query;
    query.exec("SELECT * FROM messages");
    QJsonArray arr;
    QString name;
    QString message;
    while (query.next()){
        QJsonObject list;
        name = query.value(1).toString();
        message = query.value(2).toString();
        list.insert("name", name);
        list.insert("message", message);
        arr.push_back(list);
    }
    QJsonObject json;
    json.insert("messages", arr);
    QJsonDocument doc(json);
    QString msg = doc.toJson(QJsonDocument::Compact);
    clsocket->write(msg.toLatin1());

}

void Server::onDisconnect(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    socket_ptr->close();
    qDebug() << "Client: " << clients[socket_ptr] << " has left";
    clients.remove(socket_ptr);
}

void Server::onNewMessage(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    while(socket_ptr->bytesAvailable() > 0){
        auto msg = socket_ptr->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(msg);
        QJsonObject json = doc.object();

        QJsonArray messages = json["messages"].toArray();
        for (const auto& i: messages){
            QJsonObject newobj = i.toObject();
            QSqlQuery query;
            query.prepare("INSERT INTO messages (name, message) VALUES (:name, :message)");
            query.bindValue(":name", newobj["name"].toString());
            query.bindValue(":message", newobj["message"].toString());
            if (!query.exec()){
                qDebug() << query.lastError().text();
            }
            qDebug() << newobj["name"].toString() << ": " << newobj["message"].toString();
            for (auto cl = clients.keyBegin(); cl != clients.keyEnd(); ++cl){
                (*cl)->write(msg);
            }
        }
    }
}

