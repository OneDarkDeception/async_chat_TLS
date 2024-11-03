#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <boost/asio/ssl.hpp>
#include <opensslconf.h>
#include <opensslv.h>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
class Server
{
public:
	Server(ssl::context &ssl_co, io_context& io) : acc(io,tcp::endpoint(tcp::v4(),8080)), ssl_con(ssl_co)
	{
		async_accept();
	}
	void async_accept();
	void Handle(shared_ptr<ssl::stream<tcp::socket>> sock);
	void async_read(shared_ptr<ssl::stream<tcp::socket>> sock);
	void send_to_all(shared_ptr<ssl::stream<tcp::socket>> sock, const string& msg);
private:
	vector <shared_ptr<ssl::stream<tcp::socket>>> clients;
	ssl::context &ssl_con;
	tcp::acceptor acc;
	mutex mute;
};
void Server::async_accept()
{
	lock_guard <mutex> lock_mute(mute);
	auto socket = make_shared<ssl::stream<tcp::socket>>(acc.get_executor(),ssl_con);
	acc.async_accept(socket->next_layer(), [socket, this](const boost::system::error_code& e) {
		if (!e)
		{
			socket->async_handshake(ssl::stream_base::server, [socket, this](const boost::system::error_code& e) {
				if (!e)
				{
					cout << "connected properl, the device is " << socket->next_layer().remote_endpoint() << endl;
				}
				});
			Handle(socket);
			async_accept();
		}
		});
}
void Server::send_to_all(shared_ptr<ssl::stream<tcp::socket>> sock,const string &msg)
{
	auto buf = make_shared<string>(msg);
	for (auto& client : clients)
	{
		if (client != sock)
		{
			async_write(*client, buffer(*buf), [buf,sock](const boost::system::error_code &e,size_t) {
				if (e)
				{
					cout << "error" << endl;
				}
				});
		}
	}
}
void Server::Handle(shared_ptr<ssl::stream<tcp::socket>> sock)
{
	async_read(sock);
	clients.push_back(sock);
}
void Server::async_read(shared_ptr<ssl::stream<tcp::socket>> sock)
{
	auto buf = make_shared<array<char, 2048>>();
	sock->async_read_some(buffer(*buf), [buf,this,sock](const boost::system::error_code &e,size_t length) {
		if (!e)
		{
			string b(buf->data(), length);
			cout << "Received: " << b << endl;
			send_to_all(sock,b);
		}
		async_read(sock);
		});
}

int main()
{
	io_context io;
	ssl::context ssl_con(ssl::context::tlsv12_server);
	ssl_con.use_certificate_chain_file("C:/key/server.crt");
	ssl_con.use_private_key_file("C:/key/server.key",ssl::context::pem);
	Server her(ssl_con, io);
	io.run();
}
