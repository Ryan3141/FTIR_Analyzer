#pragma once

#include <memory>

#include <QThread>
#include <QFileInfo>
#include <QString>


#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>

class Web_Listener : public QObject
{
	Q_OBJECT

public:

signals:
	void Command_Recieved( QString command );

public slots:
	void Start_Thread();

public:
	Web_Listener( QObject* parent, QFileInfo config_filename );
	void Initialize_Listening_Server();

private:
	QThread* worker_thread = nullptr;
	unsigned short port = 8080;
	std::unique_ptr<Poco::Net::ServerSocket> server_socket;
	std::unique_ptr<Poco::Net::HTTPServer> http_server;

	void Got_New_Command( QString command );
};