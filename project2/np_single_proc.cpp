
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <wait.h> 
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include<algorithm>
#include<queue>
#include<map>
#include<utility>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/types.h>
#include<netdb.h>
#include <arpa/inet.h>
#include <stack>
using namespace std;

#define SERV_IP "127.1.2.3"
#define SERV_PORT 8888
// #define MAX_CONN 31
class client
{
    public:
        int id=-1;
        int port;
        int connfd=-1;
        // int send_id=-1;
        // int rec_id=-1;
        string user_name="(no name)";
        string ip;
        char buffer[15000];
        map<int,pair<int,int>> pipe_map;
        map<int,pair<int,int>>user_pipes;
        map<string,string>env_map;
        string P = "PATH";
        string home = "bin:.";
        struct sockaddr_in socket_info;
        int job_id=0;
        int user_pipe_s=0;
        int user_pipe_r=0;

        
}clients_arr[31];
fd_set afds;  
fd_set rfds;

char* findenv(char* name)
{
    char* value = getenv(const_cast<char*>(name));
    return value;
}
void Setenv(int c_id,char* name,char* value)
{
    clients_arr[c_id].env_map[string(name)]=string(value);
    int ret = setenv(name,value,1);
    
}
void close_pipe(pair<int,int>p)
{
    close(p.first);
    close(p.second);
	return;
}

void open_pipe(int in, int out)
{
    dup2(in, STDIN_FILENO);
    dup2(out,STDOUT_FILENO);
    return;
}
char ** to_char_array(vector<string>input)
{
	char **args;
	args = new char *[input.size() + 1];
	for (int i = 0; i < input.size(); i++)
	{
		if(i==0)
		{
			// args[i] = strdup(("bin/"+input[i]).c_str());
			args[i] = strdup((input[i]).c_str());

		}
		else
		{
			args[i] = strdup((input[i]).c_str());
		}
	}
	args[input.size()] = NULL;
	return args;
}
void is_user_pipe(string input,int *s_id,int *r_id)
{
	stringstream str2(input);
    string t;
	while (str2 >> t)
	{
        if(t==">")
        {
            continue;
        }
        else if (t[0]=='>')
        {
            const char* temp=t.c_str();
            *(s_id)=atoi(temp+1);

        }
        else if(t[0]=='<')
        {
            const char* temp=t.c_str();
            *(r_id)=atoi(temp+1);
        }
	}
}
vector<string> string_processing(int c_id,string input,string &f_name,bool &redirection2)
{
	vector<string> inputv;
	stringstream str2(input);
	string t;
	bool redirection = false;
	while (str2 >> t)
	{
		if (t == ">")
		{
			redirection = true;
		}
        else if (t[0]=='>')
        {
            continue;
        }
        else if(t[0]=='<')
        {
            continue;
        }
		else
		{
			if (redirection)
			{
				f_name = t;
				redirection = false;
				redirection2 = true;
			}
			else
			{
				inputv.push_back(t);
			}
		}
	}
	return inputv;
}

void split_job(queue<string> &jobs_queue,string input_string)
{
    vector<string> inputv;
	stringstream str2(input_string);
	string token;
    string job_string;
    while (str2 >> token)
	{
		if (token[0] == '|' && token.size()>=2 || token[0]=='!')
		{
            job_string=job_string+' '+token;
            job_string=job_string.substr (1,job_string.size()-1);
            jobs_queue.push(job_string);
            job_string="";
		}
        else
        {
            job_string=job_string+' '+token;
        }
	}
    if(job_string!="")
    {
        job_string=job_string.substr (1,job_string.size()-1);
        jobs_queue.push(job_string);

    }  
    return;
}

bool is_number_pipe(string token)
{
    if(token[0]=='|'&& token.size()>1){
        return true;
    }
    else{
        return false;
    }
}
pair<int,int>create_pipe()
{ 
    int pi[2];                    
    if (pipe(pi) < 0)                        
    {
        // //cout << "pipe faild" << endl;
        exit(-1);
    }
    pair<int,int>temp;
    temp.first=pi[0];
    temp.second=pi[1];
    return temp;
}

vector<string>split_small_job(string job_string)
{
    vector<string>small_job_vec;
    stringstream str2(job_string);
    string token;
    string small_job;
    while (str2 >> token)
    {
        if(token[0]=='|'&& token.size()==1)
        {
            small_job_vec.push_back(small_job);
            small_job="";
            small_job_vec.push_back(token);
        }
        else if(is_number_pipe(token))
        {
            small_job_vec.push_back(small_job);
            small_job="";
            small_job_vec.push_back(token);   
        }
        else if(token[0]=='!'&& token.size()!=1)
        {
            small_job_vec.push_back(small_job);
            small_job="";
            small_job_vec.push_back(token);
        }
        else
        {
            if(small_job!="")
            {
                small_job=small_job+" "+token;
            }
            else
            {
                small_job=token;
            }            
        }          
    }
	if(small_job!="")
	{
		small_job_vec.push_back(small_job);
	}

    return small_job_vec;

}
void processing_small_job(int c_id,string small_job)
{
    string f_name;
    bool redirection2=false;
    vector<string> small_job_vec= string_processing(c_id,small_job,f_name,redirection2);
    char ** args=to_char_array(small_job_vec);
    string command(args[0]);
    if(command == "setenv"){
        exit(0);
    }
    else if(command == "printenv"){
        if(findenv(args[1])!=nullptr){
            cout<<findenv(args[1])<<endl;
        }
        exit(0);
    }
    if (redirection2)
    {				
        freopen(f_name.c_str(), "w", stdout); //output redirection				
    }
    if(clients_arr[c_id].user_pipe_r==-1)
    {
        // //cout<<"freopen(/dev/null,r, stdin);  "<<endl;
        freopen("/dev/null", "r", stdin);     
    }
    if(clients_arr[c_id].user_pipe_s==-1)
    {
        // //cout<<"freopen(/dev/null,w, stdin);  "<<endl;

        freopen("/dev/null", "w", stdout);  
    }
    if (execvp(args[0], args) != -1)
        ;
    else
    {
        cerr<< "Unknown command: ["  << args[0] <<"]."<< endl;
        exit(0);
    }
	return;
}
void close_number_pipes(map<int,pair<int,int>>pipe_map,int job_id,int jump_n)
{  
    for(map<int,pair<int,int>>::iterator it = pipe_map.begin(); it != pipe_map.end(); ++it)
    {
        if(it->first!=job_id && it->first!=(job_id+jump_n)) 
        {
            close_pipe(it->second);
        } 
    }
}
void job_processing(int c_id,int client_st,int i,int job_id,int jump_n ,string small_job,pair<int,int>left_pipe,pair<int,int>right_pipe,bool is_end,bool rec_number_pipe,bool is_error_pipe,map<int,pair<int,int>>&pipe_map)
{
    int id_s=-2;
    int id_r=-2;
    is_user_pipe(small_job,&id_s,&id_r);
    //cout<<id_s<<"     "<<id_r<<endl;
    pipe_map.erase(job_id);
    int status;
    pid_t pid = fork();
    while (pid < 0) 
    {
        wait(&status);
        pid = fork();
    }
    if(pid==0)
    { 
        close_number_pipes(pipe_map,job_id,jump_n);
        if(rec_number_pipe)//there exist some pipes that sent messages through number pipe to this process
        {
            if(i==-1)// only one tiny job (e.g ls)
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
                close(STDOUT_FILENO); // Close stdout
                dup2(client_st, STDOUT_FILENO); // Redirect stdout to client_fd
            }
            else if(is_end) // the last tiny job
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
                close(STDOUT_FILENO); // Close stdout
                dup2(client_st, STDOUT_FILENO); // Redirect stdout to client_fd
            }
            else if(i==0) // the first tiny job
            {
                if(is_error_pipe)
                {
                    dup2(right_pipe.second,STDERR_FILENO);
                }
                open_pipe(left_pipe.first,right_pipe.second);
                close_pipe(left_pipe);
                close_pipe(right_pipe);
            }
            else if(is_error_pipe)
            {
                dup2(right_pipe.second,STDERR_FILENO);
                open_pipe(left_pipe.first,right_pipe.second);
                close_pipe(right_pipe);
                close_pipe(left_pipe);
            }
            else
            {
                open_pipe(left_pipe.first,right_pipe.second);
                close_pipe(right_pipe);
                close_pipe(left_pipe);
            }
            if(clients_arr[c_id].user_pipe_s==1)
            {
                dup2(right_pipe.second,STDOUT_FILENO);
                close_pipe(right_pipe);
                // clients_arr[c_id].user_pipe_s=0;
            }           

        }
        else//none of pipes sent messages through number pipe to this process
        {

            if(i==-1)// only one tiny job (e.g ls)
            {
                close(STDOUT_FILENO); // Close stdout
                dup2(client_st, STDOUT_FILENO); // Redirect stdout to client_fd
            }
            else if(is_end)//the last tiny job
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
                close(STDOUT_FILENO); // Close stdout
                dup2(client_st, STDOUT_FILENO); // Redirect stdout to client_fd
            }
            else if(i==0)// the first tiny job
            {
                if(is_error_pipe)
                {
                    dup2(right_pipe.second,STDERR_FILENO);
                }
                dup2(right_pipe.second,STDOUT_FILENO);
                close_pipe(right_pipe);
            }
            else if(is_error_pipe)
            {
                dup2(right_pipe.second,STDERR_FILENO);
                open_pipe(left_pipe.first,right_pipe.second);
                close_pipe(right_pipe);
                close_pipe(left_pipe);
            }
            else
            {
                open_pipe(left_pipe.first,right_pipe.second);
                close_pipe(right_pipe);
                close_pipe(left_pipe);

            }
            if(clients_arr[c_id].user_pipe_s==1)
            {
                dup2(right_pipe.second,STDOUT_FILENO);
                close_pipe(right_pipe);
                clients_arr[c_id].user_pipe_s=0;
            }
            if(clients_arr[c_id].user_pipe_r==1)
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
                clients_arr[c_id].user_pipe_r=0;               
            }
        }
        if(!is_error_pipe)
        {
            close(STDERR_FILENO);
            dup2(client_st, STDERR_FILENO); // Redirect stdout to client_fd 
        }
        processing_small_job(c_id,small_job);                            
    }
    else//parent
    {
        //cout<<"job processing"<<endl;
        //cout<<"small_job: "<<small_job<<endl;
        //cout<<small_job<<endl;
        //cout<<"i: "<<i<<endl;
        //cout<<"user rec: "<<id_r<<endl;
        //cout<<"user send: "<<id_s<<endl;
        //cout<<"left: "<<left_pipe.first<<" "<<left_pipe.second<<endl;
        //cout<<"right: "<<right_pipe.first<<" "<<right_pipe.second<<endl;
        if(!rec_number_pipe && i==0)//the process didn't receive message form number pipe or pipe and isn't the end of the job
        {
            if(clients_arr[c_id].user_pipe_r==1)
            {
                close_pipe(left_pipe);
            }
            clients_arr[c_id].user_pipe_r=0;
            clients_arr[c_id].user_pipe_s=0;
            signal(SIGCHLD,SIG_IGN); 
            return;
            // wait(NULL);
        }
        else if(!is_end)// not the end of the job
        {
            close_pipe(left_pipe);
            clients_arr[c_id].user_pipe_r=0;
            clients_arr[c_id].user_pipe_s=0;
            signal(SIGCHLD,SIG_IGN);
            // wait(NULL);
        }
        else//the last tiny job
        {
            cout<<"elisa1"<<"!!!!!!!!!!!"<<endl;

            if(i!=-1 || rec_number_pipe || clients_arr[c_id].user_pipe_r==1)//the process receive some message from number pipe or pipe
            {
                cout<<"elisa2"<<"!!!!!!!!!!!"<<endl;
                close_pipe(left_pipe);
            }
            if (clients_arr[c_id].user_pipe_s==1)
            {
                // //cout<<"user_pipe_s==1"<<endl;
                signal(SIGCHLD,SIG_IGN);
                clients_arr[c_id].user_pipe_r=0;
                clients_arr[c_id].user_pipe_s=0;
                return;
            }
            cout<<"elisa3"<<"!!!!!!!!!!!"<<endl;
            clients_arr[c_id].user_pipe_r=0;
            clients_arr[c_id].user_pipe_s=0;
            cout<<"elisa4"<<"!!!!!!!!!!!"<<endl;
            wait(NULL);
            cout<<"elisa5"<<"!!!!!!!!!!!"<<endl;

        }
        // wait(NULL);
    }
    return;

}
void send_to_all(string client_mes)
{
    for (int i=1;i<31;i++)
    {
        if(clients_arr[i].connfd!=-1)
        {
            send(clients_arr[i].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);   
        }
    }
}
int create_read_pipe(int id_r,int c_id,string small_job)
{
    if(id_r!=-1)
    {
        if(id_r>=31)
        {
            string client_mes="*** Error: user #"+to_string(id_r) +" does not exist yet. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
            return -1;
        }
        else if(clients_arr[id_r].id==-1)
        {
            string client_mes="*** Error: user #"+to_string(id_r) +" does not exist yet. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);      
            return -1;
            
        }
        else if((clients_arr[c_id].user_pipes.find(id_r) != clients_arr[c_id].user_pipes.end()))
        {
            // clients_arr[c_id].user_pipes[id_r]=create_pipe(); 
            string client_mes="*** "+clients_arr[c_id].user_name+" (#"+to_string(c_id)+") just received from "+clients_arr[id_r].user_name+ " (#"+to_string(id_r)+") by \'"+small_job+"\' ***\n";
            send_to_all(client_mes);
            return 1;
                  
        }
        else
        {
            cout<<"pipe does not exist"<<endl;
            string client_mes="*** Error: the pipe #"+to_string(id_r)+"->#"+to_string(c_id) +" does not exist yet. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);  
            return -1;

        }
    } 
    return 0;   
}
int create_send_pipe(int id_s,int c_id,string small_job)
{
    if(id_s!=-1)
    {
        if(id_s>=31)
        {
            string client_mes="*** Error: user #"+to_string(id_s) +" does not exist yet. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
            return -1;
        }
        else if(clients_arr[id_s].id==-1)
        {
            string client_mes="*** Error: user #"+to_string(id_s) +" does not exist yet. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);      
            return -1;
            
        }
        else if((clients_arr[id_s].user_pipes.find(c_id) != clients_arr[id_s].user_pipes.end()))
        {
            string client_mes="*** Error: the pipe #"+to_string(c_id)+"->#"+to_string(id_s) +" already exists. ***\n";
            send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);  
            return -1;
                  
        }
        else
        {
            clients_arr[id_s].user_pipes[c_id]=create_pipe(); 
            string client_mes="*** "+clients_arr[c_id].user_name+" (#"+to_string(c_id)+") just piped \'"+small_job+"\' to "+clients_arr[id_s].user_name+" (#"+to_string(id_s)+") ***\n";
            send_to_all(client_mes);
            return 1;

        }
    }
    return 0;
}
pair<int,int> create_number_pipe(int job_p_jump,map<int,pair<int,int>>&pipe_map)
{
    pair<int,int>temp;
    if((pipe_map.find(job_p_jump) != pipe_map.end()))
    {
        temp.first=pipe_map[job_p_jump].first;
        temp.second=pipe_map[job_p_jump].second;
    }
    else
    {
        temp=create_pipe();
    }
    pipe_map[job_p_jump]=temp;
    return temp; 
}
pair<int,int> create_error_pipe(int job_p_jump,map<int,pair<int,int>>&pipe_map)
{
    pair<int,int>temp;
    if((pipe_map.find(job_p_jump) != pipe_map.end()))
    {
        temp.first=pipe_map[job_p_jump].first;
        temp.second=pipe_map[job_p_jump].second;
    }
    else
    {
        temp=create_pipe();
    }
    pipe_map[job_p_jump]=temp;
    return temp; 
}
int split_int(string next)
{
    string num1="";
    string num2="";
    bool plus=false;
    for(int i=1;i<next.size();i++)
    {
        if (next[i]=='+')
        {
            plus=true;
        }
        if(plus)
        {
            num2+=next[i];
        }
        else
        {
            num1+=next[i];
        }

    }
    const char* t1=num1.c_str();
    const char* t2=num2.c_str();

    int jump_n1=atoi(t1);
    int jump_n2=0;
    if(num2!="")
    {
        jump_n2=atoi(t2);
    }
    return jump_n1+jump_n2;

}
int passivesock(const char *service, const char *protocol, int qlen)
{
	struct servent *pse;			// pointer to service info entry
	struct protoent *ppe;			// pointer to protocol info entry
	struct sockaddr_in sin;			// an endpoint addr
	int sock, type;		            // socketfd & socket type

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	
	//map service name to port number
	if(pse = getservbyname(service, protocol))
		sin.sin_port = htons(ntohs((u_short) pse -> s_port));
	else if((sin.sin_port = htons((u_short)atoi(service))) == 0) {
		// perror("service entry");
		exit(-1);
	}

	//map protocol name to protocol number
	if((ppe = getprotobyname(protocol)) == 0) {
		// perror("protocol entry");
		exit(-1);
	}
	
	//use protocol to choose a socket type
	if(strcmp(protocol, "tcp") == 0)
		type = SOCK_STREAM;
	else
		type = SOCK_DGRAM;

	// allocate a socket
	sock = socket(PF_INET, type, ppe->p_proto);
	if(sock < 0) {
		exit(-1);
	}
    const int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	//bind the socket
	if(bind(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		// perror("bind");
		exit(-1);
	}
	if(type == SOCK_STREAM && listen(sock, qlen) < 0) {
		// perror("listen");
		exit(-1);
	}
	return sock;
}
int passiveTCP(const char* service, int qlen)
{
	return passivesock(service, "tcp", qlen);
}

string remove_endl(char *s)
{    char *bp;
    if((bp = strchr(s, '\n')) != NULL)
        *bp = '\0';
    if((bp = strchr(s, '\r')) != NULL)
        *bp = '\0';
    string inp=(string)(s);
    return inp;
}
void my_exit(int c_id)
{
    for(int i=1;i<31;i++)
    {
        if( (clients_arr[i].user_pipes.find(c_id) != clients_arr[i].user_pipes.end()))
        {
            close_pipe(clients_arr[i].user_pipes[c_id]);
            clients_arr[i].user_pipes.erase(c_id);
        }
    }
    for(int i=1;i<31;i++)
    {
        if(clients_arr[c_id].user_pipes.find(i)!=clients_arr[c_id].user_pipes.end())
        {
            close_pipe(clients_arr[c_id].user_pipes[i]);
        }
    }
}
void who(int c_id)
{
    string client_mes="<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    for(int i=1;i<31;i++)
    {
        if(clients_arr[i].connfd!=-1)
        {
            client_mes+=(to_string(i)+"\t"+clients_arr[i].user_name+"\t"+clients_arr[i].ip+":"+to_string(clients_arr[i].port));
            if(i==c_id)
            {
                client_mes+="\t<-me\n";
            }
            else client_mes+="\n";
        }
    }
    send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);   
}
void rename(int c_id,string name)
{
    bool name_exist=false;
    for(int j=1;j<31;j++)
    {
        if(clients_arr[j].user_name==name)
        {
            name_exist=true;
            break;
        }        
    }
    if(name_exist)
    {
        string client_mes="*** User \'"+name+"\' already exists. ***\n";
        send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);

    }
    else
    {
        clients_arr[c_id].user_name=name;
        string client_mes="*** User from "+clients_arr[c_id].ip+":"+to_string(clients_arr[c_id].port)+" is named \'"+name+"\'. ***\n";
        send_to_all(client_mes);
    }
}
void initial_client(int c_id)
{
    clients_arr[c_id].id=-1;
    clients_arr[c_id].port;
    clients_arr[c_id].connfd=-1;
    clients_arr[c_id].user_name="(no name)";
    clients_arr[c_id].ip="";
    memset(clients_arr[c_id].buffer,'\0',15000);   
    clients_arr[c_id].pipe_map.clear();
    clients_arr[c_id].user_pipes.clear();
    clients_arr[c_id].env_map.clear();
    clients_arr[c_id].P = "PATH";
    clients_arr[c_id].home = "bin:.";
    clients_arr[c_id].job_id=0;
    clients_arr[c_id].user_pipe_s=0;
    clients_arr[c_id].user_pipe_r=0;

}
void logout(int i)
{
    my_exit(i);    
    string client_mes="*** User \'"+clients_arr[i].user_name+"\' left. ***\n";
    for(int j=1;j<31;j++)
    {
        if(clients_arr[j].connfd!=-1 && i!=j)
        {
            send(clients_arr[j].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
        }
    }
    close(clients_arr[i].connfd);
    FD_CLR(clients_arr[i].connfd,&afds);
    // client temp;
    initial_client(i);
    // clients_arr[i]=temp;   
    //cout<<"clients_arr"<<clients_arr[i].id<<endl;
    //cout<<"connd"<<clients_arr[i].connfd<<endl;
}
void yell(int c_id ,string mes)
{
    string client_mes="*** "+clients_arr[c_id].user_name+" yelled ***: "+mes+"\n";
    send_to_all(client_mes);    
}
void tell(int c_id,int rec_id,string message)
{
    if(rec_id<31 && clients_arr[rec_id].connfd!=-1)
    {
        string client_mes="*** "+clients_arr[c_id].user_name+" told you ***: "+message+"\n";
        send(clients_arr[rec_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
    }
    else
    {
        string client_mes="*** Error: user #"+to_string(rec_id)+" does not exist yet. ***\n";
        send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);

    }
}
void login(int i)
{
    string client_mes="*** User \'(no name)\' entered from "+clients_arr[i].ip+":"+to_string(clients_arr[i].port)+". ***\n";
    for(int j=1;j<31;j++)
    {
        if(clients_arr[j].connfd!=-1)
        {
            send(clients_arr[j].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
        }
    }
}
void greeting(int client_st)
{
    string client_mes="****************************************\n** Welcome to the information server. **\n****************************************\n";
    send(client_st, client_mes.c_str(), strlen(client_mes.c_str()), 0);
}
void unset_env(int c_id)
{
    for(std::map<string,string>::iterator it = clients_arr[c_id].env_map.begin(); it != clients_arr[c_id].env_map.end(); ++it) 
    {
        unsetenv(it->first.c_str());
    }
}
void processing_client(int c_id)
{
    queue<string> jobs_queue;
    int jobs_id=clients_arr[c_id].job_id;
    string inp=remove_endl(clients_arr[c_id].buffer);
    split_job(jobs_queue,inp);
    while(!jobs_queue.empty())
    {
        jobs_id+=1;
        clients_arr[c_id].job_id+=1;
        pair<int,int>left_pipe,right_pipe;
        string job_string=jobs_queue.front();
        jobs_queue.pop();
        // //cout<<"########"<<job_string<<"  "<<jobs_id<<endl<<endl;
        vector<string> small_job_vec;
        small_job_vec=split_small_job(job_string);
        string small_job="";
        string next="";
        if( (clients_arr[c_id].pipe_map.find(jobs_id) != clients_arr[c_id].pipe_map.end()))//there exist some pipes that sent messages through number pipe to this process
        {
            for(int i=0;i<small_job_vec.size()-1;i++)
            {
                small_job=small_job_vec[i];
                next=small_job_vec[i+1];
                int id_s=-1;
                int id_r=-1;
                is_user_pipe(small_job,&id_s,&id_r);
                clients_arr[c_id].user_pipe_r=create_read_pipe(id_r,c_id,job_string);
                if(clients_arr[c_id].user_pipe_r==1)
                {
                    //cout<<"rec success"<<endl;
                    left_pipe=clients_arr[c_id].user_pipes[id_r];
                    clients_arr[c_id].user_pipes.erase(id_r);
                }
                clients_arr[c_id].user_pipe_s=create_send_pipe(id_s,c_id,job_string);
                if(clients_arr[c_id].user_pipe_s==1)
                {
                    //cout<<"send success"<<endl;
                    right_pipe=clients_arr[id_s].user_pipes[c_id];
                }
                stringstream str3(small_job);
                vector<string>tiny_jobs;
                string t;
                while(str3>>t)
                {
                    tiny_jobs.push_back(t);
                }
                if(tiny_jobs[0]=="setenv")
                {
                    Setenv(c_id,const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                    clients_arr[c_id].home=tiny_jobs[2];
                    clients_arr[c_id].P=tiny_jobs[1];
                    //#####################//
                    continue;
                }
                else if(!is_number_pipe(next) && next=="|")	// stdout-> pipe
                {//
                    pair<int,int>temp;
                    temp=create_pipe();
                    if(i==0)
                    {
                        left_pipe=clients_arr[c_id].pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,false,true,false,clients_arr[c_id].pipe_map);

                }
                else if(is_number_pipe(next)) //stdout->number pipe
                {
                    int jump_n=split_int(next);

                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,clients_arr[c_id].pipe_map);
                    if(i==0)
                    {
                        left_pipe=clients_arr[c_id].pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,false,clients_arr[c_id].pipe_map);
            
                } 
                else if(next[0]=='!') //stdout -> number pipe and  stderr-> number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,clients_arr[c_id].pipe_map);
                    if(i==0)
                    {
                        left_pipe=clients_arr[c_id].pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;	
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,true,clients_arr[c_id].pipe_map);
                }       
            }
            small_job=small_job_vec[small_job_vec.size()-1];
            stringstream str3(small_job);
            vector<string>tiny_jobs;
            string t;
            while(str3>>t)
            {
                tiny_jobs.push_back(t);
            }
            if(tiny_jobs[0]=="setenv")
            {
                Setenv(c_id,const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                clients_arr[c_id].home=tiny_jobs[2];
                clients_arr[c_id].P=tiny_jobs[1];
                continue;
            }
            if(small_job=="exit")
            {
                logout(c_id);
                return;
            }
            if(is_number_pipe(small_job) || small_job[0]=='!')
            {
                continue;
            }
            int i;
            if(small_job_vec.size()!=1)
            {
                left_pipe=right_pipe;
                i=-2;
            }
            else
            {
                left_pipe=clients_arr[c_id].pipe_map[jobs_id];
                i=-1;
            }
            int id_s=-1;
            int id_r=-1;
            is_user_pipe(small_job,&id_s,&id_r);
            clients_arr[c_id].user_pipe_s=create_send_pipe(id_s,c_id,job_string);
            if(clients_arr[c_id].user_pipe_s==1)
            {
                right_pipe=clients_arr[id_s].user_pipes[c_id];
            }
            // else if(id_s!=-1)
            // {
            //     continue;
            // }
            job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,true,true,false,clients_arr[c_id].pipe_map);

        }
        else//none of pipes sent messages through number pipe to this process
        {
            for(int i=0;i<small_job_vec.size()-1;i++)
            {
                small_job=small_job_vec[i];
                next=small_job_vec[i+1];
                int id_s=-1;
                int id_r=-1;
                is_user_pipe(small_job,&id_s,&id_r);
                clients_arr[c_id].user_pipe_r=create_read_pipe(id_r,c_id,job_string);
                if(clients_arr[c_id].user_pipe_r==1)
                {
                    //cout<<"rec success"<<endl;
                    left_pipe=clients_arr[c_id].user_pipes[id_r];
                    clients_arr[c_id].user_pipes.erase(id_r);
                }
                clients_arr[c_id].user_pipe_s=create_send_pipe(id_s,c_id,job_string);
                if(clients_arr[c_id].user_pipe_s==1)
                {
                    //cout<<"send success"<<endl;
                    right_pipe=clients_arr[id_s].user_pipes[c_id];
                }
                // //cout<<small_job<<endl;
                stringstream str3(small_job);
                vector<string>tiny_jobs;
                string t;
                while(str3>>t)
                {
                    tiny_jobs.push_back(t);
                }
                if(tiny_jobs[0]=="setenv")
                {
                    Setenv(c_id,const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                    clients_arr[c_id].home=tiny_jobs[2];
                    clients_arr[c_id].P=tiny_jobs[1];
                    continue;
                }
                if(tiny_jobs[0]=="tell" || tiny_jobs[0]=="yell" || tiny_jobs[0]=="name")
                {
                    //cout<<"!!!!!"<<endl;
                    while (!jobs_queue.empty()) jobs_queue.pop();
                    break;
                }
                if(tiny_jobs[0]=="who" ||tiny_jobs[0]=="exit")
                {
                    break;
                }
                else if(!is_number_pipe(next) && next=="|")//stdout -> pipe
                {
                    pair<int,int>temp;
                    temp=create_pipe();
                    if(i!=0)
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,false,false,false,clients_arr[c_id].pipe_map);
                }
                else if(is_number_pipe(next))//stdout -> number pipe
                {
                    // const char* t=next.c_str();
                    // int jump_n=atoi(t+1);
                    int jump_n=split_int(next);
                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,clients_arr[c_id].pipe_map);
                    if(i!=0)
                    {
                        left_pipe=right_pipe;
                    }                        
                    right_pipe=temp;
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,false,clients_arr[c_id].pipe_map);
                }
                else if(next[0]=='!')//stdout->number pipe and stderr->number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_error_pipe(jobs_id+jump_n,clients_arr[c_id].pipe_map);
                    if(i!=0)
                    {
                        left_pipe=right_pipe;
                    }                        
                    right_pipe=temp;	
                    job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,true,clients_arr[c_id].pipe_map);
                }

            }                
            small_job=small_job_vec[small_job_vec.size()-1];
            stringstream str3(small_job);
            vector<string>tiny_jobs;
            string t;
            while(str3>>t)
            {
                tiny_jobs.push_back(t);
            }
            if(tiny_jobs[0]=="setenv")
            {
                Setenv(c_id,const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                clients_arr[c_id].home=tiny_jobs[2];
                clients_arr[c_id].P=tiny_jobs[1];
                continue;
            }
            stringstream str4(inp);
            tiny_jobs.clear();
            while(str4>>t)
            {
                tiny_jobs.push_back(t);
            }
            if(tiny_jobs[0]=="who")
            {
                who(c_id);
                continue;
            }
            if(tiny_jobs[0]=="name")
            {
                rename(c_id,tiny_jobs[1]);
                continue;
            }
            if(tiny_jobs[0]=="yell")
            {
                const char* t=(const char*)clients_arr[c_id].buffer;
                string mes=string(t+5);
                yell(c_id,mes);
                continue;
            }
            if(tiny_jobs[0]=="tell")
            {
                string mes="";
                int space = 2;
                string tell_mes=string(clients_arr[c_id].buffer);
                for (int i = 0; i < tell_mes.size(); ++i)
                {
                    if(space == 0)
                        mes += tell_mes[i];
                    else if (tell_mes[i] == ' ') space--;
                }
                tell(c_id,stoi(tiny_jobs[1]),mes);
                continue;
            }

            if(small_job=="exit")
            {
                logout(c_id);
                return ;
            }
            if(is_number_pipe(small_job) || small_job[0]=='!')
            {
                continue;
            }
            int i;
            if(small_job_vec.size()==1)
            {
                i=-1;
            }
            else
            {
                left_pipe=right_pipe;
                right_pipe.first=0;
                right_pipe.second=0;
                i=-2;
            }
            int id_s=-1;
            int id_r=-1;
            is_user_pipe(small_job,&id_s,&id_r);
            clients_arr[c_id].user_pipe_r=create_read_pipe(id_r,c_id,job_string);
            if(clients_arr[c_id].user_pipe_r==1)
            {
                //cout<<"rec success"<<endl;
                left_pipe=clients_arr[c_id].user_pipes[id_r];
                clients_arr[c_id].user_pipes.erase(id_r);
            }
            // else if(id_r!=-1)
            // {
            //     continue;
            // }
            // else continue;
            clients_arr[c_id].user_pipe_s=create_send_pipe(id_s,c_id,job_string);
            if(clients_arr[c_id].user_pipe_s==1)
            {
                //cout<<"send success"<<endl;
                right_pipe=clients_arr[id_s].user_pipes[c_id];
            }
            // else if(id_s!=-1)
            // {
            //     continue;
            // }
            // else continue;

            job_processing(c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,true,false,false,clients_arr[c_id].pipe_map); 
        }
    }
    
}

int main(int argc, const char*argv[])
{
    int PORT=stoi(argv[1], nullptr);
    int listenfd=passiveTCP(argv[1],30);
    struct sockaddr_in servaddr,clitaddr;
    int connfd;  
    int readyfd;  
    int maxfd = 0;  
    FD_ZERO(&afds); 
    FD_ZERO(&rfds);   
    maxfd = listenfd;  
    FD_SET(listenfd,&afds);

    while(1)
    {
        // //cout<<"elisa1"<<endl;
        rfds = afds ;  
        if(readyfd = select(maxfd+1,&rfds,NULL,NULL,NULL) == -1)
        {
            continue;
        }
        if(FD_ISSET(listenfd,&rfds)) 
        {
            socklen_t addr_len = sizeof(clitaddr);
            connfd = accept(listenfd,(struct sockaddr *)&clitaddr,(socklen_t*)&addr_len); 
            if(connfd <0)
            {
                continue ;
            }
            bool have_sapce=false;
            for(int i=1;i<31;i++)
            {
                if(clients_arr[i].id == -1)
                {
                    have_sapce=true;
                    clients_arr[i].id=i;
                    clients_arr[i].connfd = connfd;
                    clients_arr[i].socket_info = clitaddr;
                    clients_arr[i].port = ntohs(clitaddr.sin_port);
                    clients_arr[i].ip=string(inet_ntoa(clitaddr.sin_addr));
                    greeting(clients_arr[i].connfd);
                    login(i);
                    clients_arr[i].env_map["PATH"]="bin:.";

                    string str = "% ";
                    send(clients_arr[i].connfd, str.c_str(), strlen(str.c_str()), 0);
                    break;
                }
            }
            if(have_sapce==false)
            {
                close(connfd);
            }
            else
            {
                FD_SET(connfd,&afds);
            }
            if(connfd>maxfd)maxfd = connfd;  

            readyfd --;
            if(readyfd == 0)continue;  
        }
        for(int i=1;i<31;i++)
        {
            if(clients_arr[i].connfd == -1)continue; 
            if(FD_ISSET(clients_arr[i].connfd,&rfds))   
            {
                int readcount = read(clients_arr[i].connfd,clients_arr[i].buffer,15000);
                if(readcount == 0)  
                {
                    my_exit(i);    
                    string client_mes="*** User \'"+clients_arr[i].user_name+"\' left. ***\n";
                    for(int j=1;j<31;j++)
                    {
                        if(clients_arr[j].connfd!=-1 && i!=j)
                        {
                            send(clients_arr[j].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
                        }
                    }
                    close(clients_arr[i].connfd);
                    FD_CLR(clients_arr[i].connfd,&afds);
                    client temp;
                    clients_arr[i]=temp;   
                    //cout<<"clients_arr"<<clients_arr[i].id<<endl;
                    //cout<<"connd"<<clients_arr[i].connfd<<endl;
                }
                else if(readcount <0)
                {
                    continue;
                }
                else
                {
                    // clearenv();
                    for(std::map<string,string>::iterator it = clients_arr[i].env_map.begin(); it != clients_arr[i].env_map.end(); ++it)
                    {
                        string name=it->first;
                        string value=it->second;
                        setenv(const_cast<char*>(name.c_str()),const_cast<char*>(value.c_str()),1);
                    }
                    processing_client(i);
                    unset_env(i);
                    memset(clients_arr[i].buffer,'\0', sizeof(clients_arr[i].buffer));
                    string str = "% ";
                    int send_ok = send(clients_arr[i].connfd, str.c_str(), strlen(str.c_str()), 0);

                }
                readyfd--;
                if(readyfd == 0)break;
            }
        }
    }
    close(listenfd);
    return 0;
}