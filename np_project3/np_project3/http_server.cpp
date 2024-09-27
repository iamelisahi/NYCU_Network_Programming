#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <regex>

using namespace std;
using boost::asio::ip::tcp;
boost::asio::io_context io_context_;

class session : public enable_shared_from_this<session> 
{
private:
    enum { max_length = 1024 };
    tcp::socket socket_;
    char data_[max_length];
    bool is_cgi = true;
	string status_str = "HTTP/1.1 200 OK\n";
	string REQUEST_METHOD;
	string REQUEST_URI;
	string QUERY_STRING;
	string SERVER_PROTOCOL;
	string HTTP_HOST;
	string SERVER_ADDR;
	string SERVER_PORT;
	string REMOTE_ADDR;
	string REMOTE_PORT;
	string exec_element;
public:
    session(tcp::socket socket) : socket_(move(socket)){}

    void start(){
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length), 
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) 
				{
                    parse();
					set_env();
                    do_write(length);
                }
        });
    }

    void parse() 
	{
		string temp(data_);
		istringstream ss(temp);
		ss >> REQUEST_METHOD >> REQUEST_URI >> SERVER_PROTOCOL >> temp >> HTTP_HOST;
		SERVER_ADDR = socket_.local_endpoint().address().to_string();
		REMOTE_ADDR = socket_.remote_endpoint().address().to_string();
		SERVER_PORT = to_string(socket_.local_endpoint().port());
		REMOTE_PORT = to_string(socket_.remote_endpoint().port());
		ss = istringstream(REQUEST_URI);
		getline(ss, REQUEST_URI, '?');
		getline(ss, QUERY_STRING);
		exec_element = boost::filesystem::current_path().string() + REQUEST_URI;
		cout << "REQUEST_METHOD: " << REQUEST_METHOD << endl;
		cout << "REQUEST_URI: " << REQUEST_URI << endl;
		cout << "QUERY_STRING: " << QUERY_STRING << endl;
		cout << "SERVER_PROTOCOL: " << SERVER_PROTOCOL << endl;
		cout << "HTTP_HOST: " << HTTP_HOST << endl;
		cout << "SERVER_ADDR: " << SERVER_ADDR << endl;
		cout << "SERVER_PORT: " << SERVER_PORT << endl;
		cout << "REMOTE_ADDR: " << REMOTE_ADDR << endl;
		cout << "REMOTE_PORT: " << REMOTE_PORT << endl;
		cout << "exec_element: " << exec_element << endl;
	}


    void set_env() 
	{
		setenv("REQUEST_METHOD", REQUEST_METHOD.c_str(), 1);
		setenv("REQUEST_URI", REQUEST_URI.c_str(), 1);
		setenv("QUERY_STRING", QUERY_STRING.c_str(), 1);
		setenv("SERVER_PROTOCOL", SERVER_PROTOCOL.c_str(), 1);
		setenv("HTTP_HOST", HTTP_HOST.c_str(), 1);
		setenv("SERVER_ADDR", SERVER_ADDR.c_str(), 1);
		setenv("SERVER_PORT", SERVER_PORT.c_str(), 1);
		setenv("REMOTE_ADDR", REMOTE_ADDR.c_str(), 1);
		setenv("REMOTE_PORT", REMOTE_PORT.c_str(), 1);
	}

	void do_write(size_t length)
	{
		cout<<"DO write!!!!!!!!!!!!!"<<endl;
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(status_str, status_str.length()),
			[this, self](boost::system::error_code ec, size_t )
			{
				if (!ec) 
				{
					io_context_.notify_fork(boost::asio::io_context::fork_prepare);
					pid_t pid = fork();
					int status;
					while (pid < 0) 
					{
						wait(&status);
						pid = fork();
					}
					if (pid != 0)
					{
						signal(SIGCHLD,SIG_IGN);
						io_context_.notify_fork(boost::asio::io_context::fork_parent);
						socket_.close();
					}
					else
					{
						io_context_.notify_fork(boost::asio::io_context::fork_child);			
						dup2(socket_.native_handle(), STDERR_FILENO);
						dup2(socket_.native_handle(), STDIN_FILENO);
						dup2(socket_.native_handle(), STDOUT_FILENO);

						socket_.close();
						string temp="printenv.cgi";

						if (execlp(exec_element.c_str(), exec_element.c_str(), NULL) < 0)
						{
							cout << "Content-type:text/html\r\n\r\n<h1>Fail</h1>";
							fflush(stdout);
						}
					}
				}

			});
	}


};

class server 
{
public:
    server(boost::asio::io_context& io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    make_shared<session>(move(socket))->start();
                }

                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) 
{
    try {
        if (argc != 2) {
            cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }
        // boost::asio::io_context io_context;
        server HttpServer(io_context_, atoi(argv[1]));
        io_context_.run();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}