#include "Web_Listener.h"

#include <iostream>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>


#include <QSettings>

using namespace Poco::Net;

class GetCommandRequestHandler : public HTTPRequestHandler
{
public:
	GetCommandRequestHandler( std::function<void( QString )> callback )
		: HTTPRequestHandler(),
		callback( callback )
	{
	}

	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		// Get the request URI
		QString uri = QString::fromStdString( request.getURI() );
		uri.replace( "%20", " " );
		if( uri != "/favicon.ico" )
			callback( uri );
		// std::cout << "Request URI: " << uri << std::endl;
		response.setChunkedTransferEncoding(true);
		response.setContentType("text/html");

		std::ostream& ostr = response.send();
		// ostr << "<html>";
		// ostr << "<head><title>Hello World!</title></head>";
		// ostr << "<body>";
		// ostr << "<h1>Hello World!</h1>";
		// ostr << "</body>";
		// ostr << "</html>";
		ostr << "<html>";
		ostr << R"web(<script>)web";
		ostr << R"web(window.onload = function(){window.close();};)web";
		ostr << "</script>";
		ostr << "</html>";
    }

	private:
		std::function<void( QString )> callback;
};

class GetCommandRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
	GetCommandRequestHandlerFactory( std::function<void( QString )> callback )
		: callback( callback )
	{
	}

	HTTPRequestHandler* createRequestHandler( const HTTPServerRequest& request )
	{
		return new GetCommandRequestHandler( callback );
	}

private:
	std::function<void( QString )> callback;
};

Web_Listener::Web_Listener( QObject* parent, QFileInfo config_filename )
	: QObject()
{
	QSettings settings( config_filename.filePath(), QSettings::IniFormat, this );
	port = settings.value( "web_listener_port", 8080 ).toInt();
}

void Web_Listener::Start_Thread()
{
	worker_thread = new QThread;
	connect( worker_thread, &QThread::started, this, &Web_Listener::Initialize_Listening_Server );
	//QTimer::singleShot( 0, this, &SQL_Manager::Initialize_DB_Connection );

	//automatically delete thread and task object when work is done:
	connect( worker_thread, &QThread::finished, this, &QObject::deleteLater );
	connect( worker_thread, &QThread::finished, worker_thread, &QObject::deleteLater );

	this->moveToThread( worker_thread );
	worker_thread->start();
}

void Web_Listener::Initialize_Listening_Server()
{
	this->server_socket = new ServerSocket( port );
	this->http_server = new HTTPServer(
		new GetCommandRequestHandlerFactory(
			[this]( QString command ){ this->Got_New_Command( command ); }),
			*server_socket, new HTTPServerParams);
	try {
		http_server->start();
		std::cout << "Server started on port " << port << "." << std::endl;
		// server.stop();
	}
	catch( Poco::Exception& exc )
	{
		std::cerr << exc.displayText() << std::endl;
	}
}

void Web_Listener::Got_New_Command( QString command )
{
	emit Command_Recieved( command );
}

int main2(int argc, char** argv)
{
	try
	{
		// Set-up a server socket
		int port = 80;
		ServerSocket socket(port);
		HTTPServer server(new GetCommandRequestHandlerFactory([]( QString ){} ), socket, new HTTPServerParams);
		server.start();
		std::cout << "Server started on port " << port<< "." << std::endl;
		while( true )
		{
			Poco::Thread::sleep(1000);
		}
		std::cout << "Stopping server..." << std::endl;
		server.stop();
	}
	catch (Poco::Exception& exc)
	{
		std::cerr << exc.displayText() << std::endl;
		return 1;
	}
	return 0;
}
