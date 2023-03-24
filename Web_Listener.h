#pragma once

#include <memory>

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QObject>
//#include <Poco/Net/HTTPServer.h>
//#include <Poco/Net/ServerSocket.h>

namespace Poco {
namespace Net{
	class ServerSocket;
	class HTTPServer;
}
}

class Web_Listener :
	public QObject
{
	Q_OBJECT

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
	Poco::Net::ServerSocket* server_socket = nullptr;
	Poco::Net::HTTPServer* http_server = nullptr;

	void Got_New_Command( QString command );
};