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
    connect(clsocket, &QTcpSocket::readyRead,
            this, &Server::onNewMessage);
    connect(clsocket, &QTcpSocket::disconnected,
            this, &Server::onDisconnect);

    QSqlQuery query;
    query.exec("SELECT * FROM messages");
    QJsonArray arr;
    QString name;
    QString message;//рассылаем все прошлые сообщения новому пользователю
    while (query.next()){
        QJsonObject list;
        name = query.value(1).toString();
        message = query.value(2).toString();
        list.insert("name", name);
        list.insert("message", message);
        arr.push_back(list);
    }
    QJsonObject json;
    json.insert("mode", "message");
    json.insert("messages", arr);
    QJsonArray arr2;
    auto lst = clients.values();
    for(const auto& i: lst){
        arr2.append(i);
    }
    QJsonObject userlist;
    userlist.insert("mode", "add_user");
    userlist.insert("names", arr2);
    QJsonObject content;//рассылаем имена пользователей в сети новому пользователю
    QJsonArray contarr;
    contarr.append(json);
    contarr.append(userlist);
    QJsonObject obj;
    obj.insert("contents", contarr);
    QJsonDocument doc(obj);
    QString str = doc.toJson(QJsonDocument::Compact);
    clsocket->write(str.toUtf8());
}
// current messages struct
//     {"contents": [
//         {mode: ""...}
//         ...
// ]}
void Server::onDisconnect(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    qDebug() << "Client: " << clients[socket_ptr] << " has left";
    clients.remove(socket_ptr);
    socket_ptr->close();
}

void Server::onNewMessage(){
    auto socket_ptr = dynamic_cast<QTcpSocket*>(sender());
    while(socket_ptr->bytesAvailable() > 0){
        auto msg = socket_ptr->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(msg);
        QJsonObject content = doc.object();
        QJsonArray contents = content["contents"].toArray();
        for(size_t i = 0; i < contents.size(); ++i){
            QJsonObject json = contents[i].toObject();
            if (json["mode"].toString() == "setName"){
                clients[socket_ptr] = json["name"].toString();
                QJsonArray arr;
                arr.append(json["name"].toString());
                QJsonObject userlist;
                userlist.insert("mode", "add_user");
                userlist.insert("names", arr);
                QJsonObject content;
                QJsonArray contarr;
                contarr.append(userlist);
                QJsonObject obj;
                obj.insert("contents", contarr);
                QJsonDocument doc(obj);
                QString str = doc.toJson(QJsonDocument::Compact);
                for (auto cl = clients.keyBegin(); cl != clients.keyEnd(); ++cl){
                    if(*cl != socket_ptr){
                        (*cl)->write(str.toUtf8());
                        qDebug() << "name sent";
                    }
                }
            }else{
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
                        qDebug() << "message sent";
                    }
                }
            }
        }
    }
}



