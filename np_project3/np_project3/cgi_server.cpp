
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
using boost::asio::ip::tcp;
using namespace std;
boost::asio::io_context io_context_;
#define max_length 15000
class client
{
public:
	client( int port, std::string addr, std::string file)
    : serverPort(port), serverAddr(addr), testFile(file){}

public:
	int serverPort;
	std::string serverAddr;
	std::string testFile;
};
class console : public enable_shared_from_this<console> 
{
   public:
    shared_ptr<tcp::socket> client_;
    char input[max_length];
    tcp::socket socket_;
    tcp::endpoint endpoint_;
    vector<string> file;
    string index;

    console(tcp::socket socket, tcp::endpoint endpoint, string filename, string idx,shared_ptr<tcp::socket> old_client)
    : socket_(move(socket)), client_(old_client) 
    {
        endpoint_ = endpoint;
        file = get_file_content(filename);
        index = idx;
        memset(input, '\0', max_length);
        
    }
    void do_write_html(string client_str) 
    {
        auto self(shared_from_this());
        boost::asio::async_write(
            * client_, boost::asio::buffer(client_str, client_str.length()),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {

                }
            }
        );
    }

    void start() 
    {
        auto self(shared_from_this());
        socket_.async_connect(
            endpoint_,
            [this, self](const boost::system::error_code& error)
            {
                if(!error) 
                {
                    do_read();
                }
            }
        );
    }

    void do_read() 
    {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(input, max_length),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    string data = string(input);
                    // cout<<data<<endl;
                    memset(input, '\0', max_length);
                    output_shell(data);
                    if(data.find("%") != std::string::npos) { // contain % it means i can write to remote
                        do_write();
                    }
                    else{
                        do_read();
                    }
                }
            }
        );
    }
    void do_write() {
        auto self(shared_from_this());
        char output[max_length];
        string cmd = file[0];
        file.erase(file.begin());
        strcpy(output, cmd.c_str());
        output_command(cmd);
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self, cmd](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    do_read();
                    if (cmd.find("exit") != string::npos) {
                        socket_.close();
                        cout << client_.use_count() << endl;
                        if(client_.use_count() == 2){
                            client_->close();
                        }
                    }
                    else {
                        do_read();
                    }
                }
            }
        );
    }
    void output_shell(string str) 
    {
        escape(str);
        string client_str="<script>document.getElementById(\'s" + index + "\').innerHTML += \'" + str + "\';</script><br>";
        do_write_html(client_str);
    }
    void output_command(string str) 
    {
        escape(str);
        string client_str="<script>document.getElementById(\'s" + index + "\').innerHTML += \'<b>" + str + "</b>\';</script><br>";
        do_write_html(client_str);

    }
    void escape(string& str) 
    {
        boost::replace_all(str, "&", "&amp;");
        boost::replace_all(str, "\"", "&quot;");
        boost::replace_all(str, "\'", "&apos;");
        boost::replace_all(str, "<", "&lt;");
        boost::replace_all(str, ">", "&gt;");
        boost::replace_all(str, "\n", "<br>");
        boost::replace_all(str, "\r", "");
    }
    vector<string> get_file_content(string testFile) 
    {
        std::ifstream in(testFile);
        char buffer[max_length];
        std::vector<std::string> res;

        memset(buffer, '\0', max_length);

        if (in.is_open())
        {	
            while(in.getline(buffer, max_length))
            {
                res.push_back(std::string(buffer) + "\n");
                memset(buffer, '\0', max_length);
            }	
        }

	    return res;
    }
};
class session : public enable_shared_from_this<session> 
{
private:
    vector<client>clients_arr;
    char data_[max_length];
    bool is_cgi = true;
	string status_str = "HTTP/1.1 200 OK\n";
	string REQUEST_METHOD="";
	string REQUEST_URI="";
	string QUERY_STRING="";
	string SERVER_PROTOCOL="";
	string HTTP_HOST="";
	string SERVER_ADDR="";
	string SERVER_PORT="";
	string REMOTE_ADDR="";
	string REMOTE_PORT="";
public:

    tcp::socket socket_;
    tcp::endpoint endpoint_;
    vector<string> file;
    string index;
    session(tcp::socket socket) : socket_(move(socket)){}
    void start()
    {
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
                    do_write(length);
                }
        });
    }
    void do_write_html(string client_str) 
    {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(client_str, client_str.length()),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    cout<<"success"<<endl;
                }
            }
        );
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
	}
    vector<string> get_file_content(string testFile) 
    {
        std::ifstream in(testFile);
        char buffer[max_length];
        std::vector<std::string> res;

        memset(buffer, '\0', max_length);

        if (in.is_open())
        {	
            while(in.getline(buffer, max_length))
            {
                res.push_back(std::string(buffer) + "\n");
                memset(buffer, '\0', max_length);
            }	
        }
	    return res;
    }
    string HTML_panel()
    {
        string str =  R""""(<!DOCTYPE html>
    <html lang="en">
    <head>
        <title>NP Project 3 Panel</title>
        <link
        rel="stylesheet"
        href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
        integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
        crossorigin="anonymous"
        />
        <link
        href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
        rel="stylesheet"
        />
        <link
        rel="icon"
        type="image/png"
        href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
        />
        <style>
        * {
            font-family: 'Source Code Pro', monospace;
        }
        </style>
    </head>
    <body class="bg-secondary pt-5">
        <form action="console.cgi" method="GET">
        <table class="table mx-auto bg-light" style="width: inherit">
            <thead class="thead-dark">
            <tr>
                <th scope="col">#</th>
                <th scope="col">Host</th>
                <th scope="col">Port</th>
                <th scope="col">Input File</th>
            </tr>
            </thead>
            <tbody>
            <tr>
                <th scope="row" class="align-middle">Session 1</th>
                <td>
                <div class="input-group">
                    <select name="h0" class="custom-select">
                    <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                    </select>
                    <div class="input-group-append">
                    <span class="input-group-text">.cs.nctu.edu.tw</span>
                    </div>
                </div>
                </td>
                <td>
                <input name="p0" type="text" class="form-control" size="5" />
                </td>
                <td>
                <select name="f0" class="custom-select">
                    <option></option>
                    <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                </select>
                </td>
            </tr>
            <tr>
                <th scope="row" class="align-middle">Session 2</th>
                <td>
                <div class="input-group">
                    <select name="h1" class="custom-select">
                    <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                    </select>
                    <div class="input-group-append">
                    <span class="input-group-text">.cs.nctu.edu.tw</span>
                    </div>
                </div>
                </td>
                <td>
                <input name="p1" type="text" class="form-control" size="5" />
                </td>
                <td>
                <select name="f1" class="custom-select">
                    <option></option>
                    <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                </select>
                </td>
            </tr>
            <tr>
                <th scope="row" class="align-middle">Session 3</th>
                <td>
                <div class="input-group">
                    <select name="h2" class="custom-select">
                    <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                    </select>
                    <div class="input-group-append">
                    <span class="input-group-text">.cs.nctu.edu.tw</span>
                    </div>
                </div>
                </td>
                <td>
                <input name="p2" type="text" class="form-control" size="5" />
                </td>
                <td>
                <select name="f2" class="custom-select">
                    <option></option>
                    <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                </select>
                </td>
            </tr>
            <tr>
                <th scope="row" class="align-middle">Session 4</th>
                <td>
                <div class="input-group">
                    <select name="h3" class="custom-select">
                    <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                    </select>
                    <div class="input-group-append">
                    <span class="input-group-text">.cs.nctu.edu.tw</span>
                    </div>
                </div>
                </td>
                <td>
                <input name="p3" type="text" class="form-control" size="5" />
                </td>
                <td>
                <select name="f3" class="custom-select">
                    <option></option>
                    <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                </select>
                </td>
            </tr>
            <tr>
                <th scope="row" class="align-middle">Session 5</th>
                <td>
                <div class="input-group">
                    <select name="h4" class="custom-select">
                    <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                    </select>
                    <div class="input-group-append">
                    <span class="input-group-text">.cs.nctu.edu.tw</span>
                    </div>
                </div>
                </td>
                <td>
                <input name="p4" type="text" class="form-control" size="5" />
                </td>
                <td>
                <select name="f4" class="custom-select">
                    <option></option>
                    <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                </select>
                </td>
            </tr>
            <tr>
                <td colspan="3"></td>
                <td>
                <button type="submit" class="btn btn-info btn-block">Run</button>
                </td>
            </tr>
            </tbody>
        </table>
        </form>
    </body>
    </html>)"""";
    return str;

    }

	void do_write(size_t length)
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(status_str, status_str.length()),
			[this, self](boost::system::error_code ec, size_t )
			{
				if (!ec) 
				{
                    if(REQUEST_URI.find("panel.cgi") != string::npos) 
                    {
                        char output[max_length];
                        file = get_file_content("panel.html");
                        strcpy(output, "Content-type: text/html\r\n\r\n");
                        boost::asio::async_write(
                            socket_, boost::asio::buffer(output, strlen(output)),
                            [this, self](boost::system::error_code ec, size_t /*length*/) {
                                if (!ec) {
                                }
                            }
                        );
                        do_write_html(HTML_panel());
                    }
                    else if(REQUEST_URI.find("console.cgi") != string::npos) 
                    {
                        init_clients();
                        print_html();
                        LinktoServer();
                    }
				}
			});
	}
    void print_html() 
    {

        string temp= "Content-type: text/html\r\n\r\n";
        string html_s= R""""(<!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8" />
        <title>NP Project 3 Sample Console</title>
        <link
        rel="stylesheet"
        href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
        integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
        crossorigin="anonymous"
        />
        <link
        href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
        rel="stylesheet"
        />
        <link
        rel="icon"
        type="image/png"
        href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
        />
        <style>
        * {
            font-family: 'Source Code Pro', monospace;
            font-size: 1rem !important;
        }
        body {
            background-color: #212529;
        }
        pre {
            color: #cccccc;
        }
        b {
            color: #01b468;
        }
        </style>
    </head>
    <body>
        <table class="table table-dark table-bordered">
        <thead>
            <tr>)"""";
        for(int i=0;i<clients_arr.size();i++)
        {
            html_s+="<th scope=\"col\">"+clients_arr[i].serverAddr+":"+to_string(clients_arr[i].serverPort)+"</th>";
        }
        html_s+=         "</tr>\
                    </thead>\
                    <tbody>\
                        <tr>";
        for(int i=0;i<clients_arr.size();i++)
        {
            html_s+="<td><pre id=\"s"+to_string(i)+"\" class=\"mb-0\"></pre></td>";
        }
        html_s+=         "</tr>\
                    </tbody>\
                </table>\
            </body>\
        </html>";
        do_write_html(temp);
        do_write_html(html_s);
    }
    void LinktoServer()
    {
        shared_ptr<tcp::socket> ptrr(&socket_);
        for (int i = 0; i < (int)clients_arr.size(); ++i)
        {
            tcp::resolver resolver(io_context_);
            tcp::resolver::query query(clients_arr[i].serverAddr, to_string(clients_arr[i].serverPort));
            tcp::resolver::iterator iter = resolver.resolve(query);
            tcp::endpoint endpoint = iter->endpoint();
            tcp::socket socket(io_context_);
            make_shared<console>(move(socket), endpoint, clients_arr[i].testFile, to_string(i),ptrr)->start();
        }
        try {
            io_context_.run();
        }
        catch (exception& e) {
            cerr << "Exception: " << e.what() << "\n";
        }
    }
    void init_clients()
    {
        string buffer=string(QUERY_STRING);
        if (buffer.length() == 0) return;
        replace(buffer.begin(), buffer.end(), '&', ' ');
        istringstream ss(buffer);
        for (int i = 0; i < 5; ++i)
        {
            string temp,addr,port,file;
            ss >> temp;
            if (temp.length() != 0) addr = temp.substr(3, temp.length() - 3);
            ss >> temp;
            if (temp.length() != 0) port = temp.substr(3, temp.length() - 3);
            ss >> temp;
            if (temp.length() != 0) file = temp.substr(3, temp.length() - 3);
            if (addr.length() != 0)
            {
                clients_arr.push_back(client( stoi(port), addr, "test_case/"+file));
            }
            else
            {
                break;
            }
        }        
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