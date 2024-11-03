#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <opensslconf.h>
#include <boost/asio/ssl.hpp>
#include <opensslv.h>
#include <chrono>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
void asyncwrite(shared_ptr<ssl::stream<tcp::socket>> sock, const string& msg)
{
    auto bufferms = make_shared<string>(msg);
    async_write(*sock, buffer(*bufferms), [msg, bufferms, sock](const boost::system::error_code& e, size_t bytes) {
        if (e)
        {
            cout << "error: " << e.message() << endl;
        }
        else {
        }
        });
}
void asyncread(shared_ptr < ssl::stream<tcp::socket>> sock)
{
    auto bufr = make_shared<array<char, 2048>>();
    sock->async_read_some(buffer(*bufr), [sock, bufr](const boost::system::error_code& e, size_t length) {
        if (!e)
        {
            cout << "received: " << string(bufr->data(), length) << endl;
            asyncread(sock);
        }
        else {
            cout << "error" << endl;
        }
        });
}
int main() {
    try {
        ssl::context ssl_con(ssl::context::tlsv12_client);
        io_context io;
        tcp::resolver res(io);
        tcp::resolver::results_type endpoint = res.resolve("172.20.10.10","8080");
        ssl_con.load_verify_file("C:/key/server.crt");
        auto sock = make_shared<ssl::stream<tcp::socket>>(io,ssl_con);
        async_connect(sock->next_layer(), endpoint, [sock](const boost::system::error_code &e,tcp::endpoint endpoint) {
            if (!e)
            {
                sock->async_handshake(ssl::stream_base::client, [sock](const boost::system::error_code &e) {
                    if (!e)
                    {
                        cout << "connected to the server" << endl;
                        asyncread(sock);
                    }
                    });
            }
            });
        thread([sock] {
            while (true)
            {
                string words;
                getline(cin, words);
                asyncwrite(sock, words);

            }
            }).detach();
        io.run();
    }
    catch (const boost::system::system_error& e) {
        cout << "System error: " << e.what() << endl;
    }
}
