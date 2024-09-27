#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string/classification.hpp>  // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp>  // Include for boost::split
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <array>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
using boost::asio::ip::tcp;
using namespace std;
boost::asio::io_context io_context_;
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

vector<client>clients_arr;
class session : public enable_shared_from_this<session> 
{
   public:
    tcp::socket socket_;
    tcp::endpoint endpoint_;
    enum { max_length = 15000 };
    char input[max_length];
    char output[max_length];
    vector<string> file;
    string index;

    session(tcp::socket socket, tcp::endpoint endpoint, string filename, string idx)
    : socket_(move(socket)) 
    {
        memset(input, '\0', max_length);
        memset(output, '\0', max_length);
        endpoint_ = endpoint;
        file = get_file_content(filename);
        index = idx;
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
            [this, self](boost::system::error_code ec, size_t length) 
            {
                if (!ec) 
                {
                    string data = string(input);
                    memset(input, '\0', max_length);
                    memset(output, '\0', max_length);
                    output_shell(data);
                    if(data.find("%") != std::string::npos) 
                    { // contain %
                        do_write();
                    }
                    else
                    {
                        do_read();
                    }
                }
            }
        );
    }

    void do_write() 
    {
        auto self(shared_from_this());
        string cmd = file[0];
        file.erase(file.begin());
        strcpy(output, cmd.c_str());
        // cout << cmd;
        output_command(cmd);
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self](boost::system::error_code ec, size_t /*length*/) 
            {
                if (!ec) 
                {
                    do_read();
                }
            }
        );
    }

    void output_shell(string str) 
    {
        escape(str);
        cout << "<script>document.getElementById(\'s" + index + "\').innerHTML += \'" + str + "\';</script>" << flush;
    }

    void output_command(string str) 
    {
        escape(str);
        cout << "<script>document.getElementById(\'s" + index + "\').innerHTML += \'<b>" + str + "</b>\';</script>" << flush;
    }

    void escape(string& str) 
    {
        boost::replace_all(str, "&", "&amp;");
        boost::replace_all(str, "\"", "&quot;");
        boost::replace_all(str, "\'", "&apos;");
        boost::replace_all(str, "<", "&lt;");
        boost::replace_all(str, ">", "&gt;");
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
    }


    vector<string> get_file_content(string testFile) 
    {
        std::ifstream in(testFile);
        char buffer[10240];
        std::vector<std::string> res;

        memset(buffer, 0, 10240);

        if (in.is_open())
        {	
            while(in.getline(buffer, 10240))
            {
                res.push_back(std::string(buffer) + "\n");
                memset(buffer, 0, 10240);
            }	
        }

	    return res;
    }
};

void print_html() 
{
    cout << "Content-type: text/html\r\n\r\n";
    cout << R""""(<!DOCTYPE html>
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
        cout<<"<th scope=\"col\">"+clients_arr[i].serverAddr+":"+to_string(clients_arr[i].serverPort)+"</th>";
    }
    cout <<         "</tr>\
                </thead>\
                <tbody>\
                    <tr>";
    for(int i=0;i<clients_arr.size();i++)
    {
        cout<<"<td><pre id=\"s"+to_string(i)+"\" class=\"mb-0\"></pre></td>";
    }
    cout <<         "</tr>\
                </tbody>\
            </table>\
        </body>\
    </html>";
}

void LinktoServer()
{
	for (int i = 0; i < (int)clients_arr.size(); ++i)
	{
        tcp::resolver resolver(io_context_);
        tcp::resolver::query query(clients_arr[i].serverAddr, to_string(clients_arr[i].serverPort));
        tcp::resolver::iterator iter = resolver.resolve(query);
        tcp::endpoint endpoint = iter->endpoint();
        tcp::socket socket(io_context_);
        make_shared<session>(move(socket), endpoint, clients_arr[i].testFile, to_string(i))->start();
	}
}
void init_clients()
{
    string QUERY_STRING = getenv("QUERY_STRING");
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
int main() 
{
    try 
    {
        init_clients();
        print_html();
        LinktoServer();
        io_context_.run();
    }
    catch (exception& e) 
    {
        cerr << e.what() << endl;
    }
}

