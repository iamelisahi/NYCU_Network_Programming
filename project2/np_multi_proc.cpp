#include <iostream>	
#include <signal.h>
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
#include <semaphore.h>	
#include <stdio.h>	
#include <stdio.h>	
#include <stdlib.h>	
#include <stack>	
#include<sys/shm.h>

// #include <fcntl.h>
using namespace std;
class client
{
    public:
        int id=-1;
        int port;
        int connfd=-1;
        char user_name[21];
        // string user_name="(no name)";
        char ip[21];
        pid_t p_id;
        char message[1500];
        sem_t sem_user_p;
        sem_t sem_write;
        int user_pipes[31];
        int receive_from;
        // char P[100]; //= "PATH".c_str();
        // char home[100]; //= "bin:.".c_str();        
};
client *clients_arr ;
int shm_clients_arr_id;
int client_id;
int jobs_id=0;
map<int,pair<int,int>>pipe_map;
int rec_fd[31];
string remove_endl(char *s)
{    char *bp;
    if((bp = strchr(s, '\n')) != NULL)
        *bp = '\0';
    if((bp = strchr(s, '\r')) != NULL)
        *bp = '\0';
    string inp=(string)(s);
    return inp;
}
void mysignalHandler( int signum ) 
{
    if(signum==SIGUSR1)
    {
        string client_mes=string(clients_arr[client_id].message);
        // //cout<<client_id<<" : "<<client_mes<<endl;
        send(clients_arr[client_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0); 
        memset(clients_arr[client_id].message,'\0',1500);      
        sem_post(&clients_arr[client_id].sem_write);
    }
    else if (signum==SIGINT)
    {
        //cout<<"!!!!!!!!!!!!!!!!!!!SIGINT!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
        shmdt(clients_arr);
        shmctl(shm_clients_arr_id, IPC_RMID, 0);
        exit(0);
        // return 0;
        
    }
    else if(signum==SIGUSR2)
    {
        string fifo_name="user_pipe/fifo_"+to_string(clients_arr[client_id].receive_from)+"_"+to_string(client_id);
        mkfifo(fifo_name.c_str(),0666);
        int fd;
        fd = open(fifo_name.c_str(), O_RDONLY);
        //cout<<"fd "<<fd<<endl;
        rec_fd[clients_arr[client_id].receive_from]=fd;
        sem_post(&clients_arr[client_id].sem_user_p);       

    }
    // cleanup and close up stuff here  
   // terminate program  

//    exit(signum);  
}
void create_user_FIFO(int c_id,int rec_id)
{
    string fifo_name="user_pipe/fifo_"+to_string(c_id)+"_"+to_string(rec_id);
    mkfifo(fifo_name.c_str(),0666);
}
void write_user_FIFO(int c_id,int rec_id)
{
    string fifo_name="user_pipe/fifo_"+to_string(c_id)+"_"+to_string(rec_id);
    int fd;
    fd = open(fifo_name.c_str(), O_WRONLY);
    dup2(fd,STDOUT_FILENO);
    close(fd);

}
void read_user_FIFO(int c_id,int id_r)
{
    // string fifo_name="/user_pipe/fifo_"+to_string(id_r)+"_"+to_string(c_id);
    // int fd;
    // fd = open(fifo_name.c_str(), O_RDONLY|O_NONBLOCK);
    // dup2(fd,STDIN_FILENO);
    dup2(rec_fd[id_r],STDIN_FILENO);
    close(rec_fd[id_r]);
    rec_fd[id_r]=0;
    sem_wait(&clients_arr[c_id].sem_user_p);
    clients_arr[c_id].user_pipes[id_r]=-1;
    sem_post(&clients_arr[c_id].sem_user_p);
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
void send_mes(int i,string client_mes)
{
    sem_wait(&clients_arr[i].sem_write);
    strcpy(clients_arr[i].message,client_mes.c_str());
    kill(clients_arr[i].p_id,SIGUSR1);
}
pair<int,int>create_pipe()
{ 
    int pi[2];                    
    if (pipe(pi) < 0)                        
    {
        exit(-1);
    }
    pair<int,int>temp;
    temp.first=pi[0];
    temp.second=pi[1];
    return temp;
}
char* findenv(char* name)
{
    char* value = getenv(const_cast<char*>(name));
    return value;
}
void Setenv(char* name,char* value)
{
    // clients_arr[c_id].env_map[string(name)]=string(value);
    int ret = setenv(name,value,1);
    
}
void send_to_all(string client_mes)
{
    
    for (int i=1;i<31;i++)
    {
        if(clients_arr[i].connfd!=-1)
        {
            send_mes(i,client_mes);
            // send(clients_arr[i].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);   
        }
    }
}
void login(int i)
{
    string client_mes="*** User \'(no name)\' entered from "+string(clients_arr[i].ip)+":"+to_string(clients_arr[i].port)+". ***\n";
    send_to_all(client_mes);

}
void greeting(int client_st)
{
    string client_mes="****************************************\n** Welcome to the information server. **\n****************************************\n";
    send_mes(client_id,client_mes);
    // send(client_st, client_mes.c_str(), strlen(client_mes.c_str()), 0);
}
void initial_client(int i)
{   

    clients_arr[i].id=-1;
    clients_arr[i].connfd=-1;
    clients_arr[i].receive_from=0;
    clients_arr[i].port=0;
    memset(clients_arr[i].message,'\0', 1500);
    memset(clients_arr[i].ip,'\0', 21);
    // memset(clients_arr[i].user_name,'\0', sizeof(clients_arr[i].user_name));
    strcpy(clients_arr[i].user_name,"(no name)");
    int sts1 = sem_init(&(clients_arr[i].sem_user_p), 1, 1);
    int sts2 = sem_init(&(clients_arr[i].sem_write), 1, 1);
    memset(clients_arr[i].user_pipes,-1,31);

}

void close_pipe(pair<int,int>p)
{
    close(p.first);
    close(p.second);
	return;
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
int create_read_pipe(int id_r,int c_id,string small_job)
{
    // //cout<<"clients_arr[c_id].user_pipes[id_r] = "<<clients_arr[c_id].user_pipes[id_r]<<endl;
    //cout<<"idrrrrrrrrrrr"<<id_r<<endl;
    if(id_r!=-2)
    {
        if(id_r>=31)
        {
            string client_mes="*** Error: user #"+to_string(id_r) +" does not exist yet. ***\n";
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
            send_mes(c_id,client_mes);
            return -1;
        }
        else if(clients_arr[id_r].id==-1)
        {
            string client_mes="*** Error: user #"+to_string(id_r) +" does not exist yet. ***\n";
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);    
            send_mes(c_id,client_mes);
            return -1;            
        }
        else if(clients_arr[c_id].user_pipes[id_r] != -1)
        {
            // clients_arr[c_id].user_pipes[id_r]=create_pipe(); 
            string client_mes="*** "+string(clients_arr[c_id].user_name)+" (#"+to_string(c_id)+") just received from "+string(clients_arr[id_r].user_name)+ " (#"+to_string(id_r)+") by \'"+small_job+"\' ***\n";
            send_to_all(client_mes);
            return 1;                  
        }
        else
        {
            string client_mes="*** Error: the pipe #"+to_string(id_r)+"->#"+to_string(c_id) +" does not exist yet. ***\n";
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);  
            send_mes(c_id,client_mes);
            return -1;
        }
    } 
    //cout<<"!!!!"<<endl;
    return 0;   
}
int create_send_pipe(int id_s,int c_id,string small_job)
{
    if(id_s!=-2)
    {
        if(id_s>=31)
        {
            string client_mes="*** Error: user #"+to_string(id_s) +" does not exist yet. ***\n";
            //cout<<client_mes<<endl;
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
            send_mes(c_id,client_mes);
            return -1;
        }
        else if(clients_arr[id_s].id==-1)
        {
            string client_mes="*** Error: user #"+to_string(id_s) +" does not exist yet. ***\n";
            //cout<<client_mes<<endl;
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0); 
            send_mes(c_id,client_mes);   
            return -1;
            
        }
        else if(clients_arr[id_s].user_pipes[c_id] != -1)
        {
            string client_mes="*** Error: the pipe #"+to_string(c_id)+"->#"+to_string(id_s) +" already exists. ***\n";
            //cout<<client_mes<<endl;
            // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);  
            send_mes(c_id,client_mes);
            return -1;
                  
        }
        else
        {
            //cout<<"send mes"<<endl;
            string client_mes="*** "+string(clients_arr[c_id].user_name)+" (#"+to_string(c_id)+") just piped \'"+small_job+"\' to "+string(clients_arr[id_s].user_name)+" (#"+to_string(id_s)+") ***\n";
            send_to_all(client_mes);
            sem_wait(&clients_arr[id_s].sem_user_p);
            clients_arr[id_s].receive_from=c_id; //FIFO
            clients_arr[id_s].user_pipes[c_id]=1;
            // sem_post(&clients_arr[id_s].sem_user_p);       
            // //cout<<"clients_arr[id_s].user_pipes[c_id]<<endl: "<<clients_arr[id_s].user_pipes[c_id]<<endl;
            //cout<<"sig send to "<<id_s<<endl;
            kill(clients_arr[id_s].p_id,SIGUSR2);
            create_user_FIFO(c_id,id_s);
            return 1;

        }
    }
    return 0;
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
void processing_small_job(int c_id,string small_job,int user_pipe_r,int user_pipe_s)
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
            send_mes(c_id,string(findenv(args[1]))+"\n");
        }
        exit(0);
    }
    if (redirection2)
    {				
        freopen(f_name.c_str(), "w", stdout); //output redirection				
    }
    if(user_pipe_r==-1)
    {
        freopen("/dev/null", "r", stdin);     
    }
    if(user_pipe_s==-1)
    {
        freopen("/dev/null", "w", stdout);  
    }
    shmdt(clients_arr);
    if (execvp(args[0], args) != -1)
        ;
    else
    {
        cerr<< "Unknown command: ["  << args[0] <<"]."<< endl;
        exit(0);
    }
	return;
}
void job_processing(int user_pipe_r,int user_pipe_s,int c_id,int client_st,int i,int job_id,int jump_n ,string small_job,pair<int,int>left_pipe,pair<int,int>right_pipe,bool is_end,bool rec_number_pipe,bool is_error_pipe,map<int,pair<int,int>>&pipe_map)
{
    int id_s=-2;
    int id_r=-2;
    is_user_pipe(small_job,&id_s,&id_r);
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
            if(user_pipe_s==1)
            {
                // dup2(right_pipe.second,STDOUT_FILENO);//FIFO
                // close_pipe(right_pipe);
                write_user_FIFO(c_id,id_s);
                // user_pipe_s=0;
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
            if(user_pipe_s==1)
            {
                // dup2(right_pipe.second,STDOUT_FILENO);
                // close_pipe(right_pipe);
                // user_pipe_s=0;
                write_user_FIFO(c_id,id_s); //FIFO
            }
            if(user_pipe_r==1)
            {
                // dup2(left_pipe.first,STDIN_FILENO);
                // close_pipe(left_pipe);
                // user_pipe_r=0;
                read_user_FIFO(c_id,id_r);               
            }
        }
        if(!is_error_pipe)
        {
            close(STDERR_FILENO);
            dup2(client_st, STDERR_FILENO); // Redirect stdout to client_fd 
        }
        processing_small_job(c_id,small_job,user_pipe_r,user_pipe_s);                            
    }
    else//parent
    {
        //cout<<"job processing"<<endl;
        //cout<<"small_job: "<<small_job<<" pid "<<pid<<endl;
        //cout<<"i: "<<i<<endl;
        //cout<<"user rec: "<<id_r<<endl;
        //cout<<"user send: "<<id_s<<endl;
        //cout<<"left: "<<left_pipe.first<<" "<<left_pipe.second<<endl;
        //cout<<"right: "<<right_pipe.first<<" "<<right_pipe.second<<endl;
        //cout<<"user_pipe_s: "<<user_pipe_s<<endl;
        if(user_pipe_r==1)
        {
            close(rec_fd[id_r]);
        }
        if(!rec_number_pipe && i==0)//the process didn't receive message form number pipe or pipe and isn't the end of the job
        {
            user_pipe_r=0;
            user_pipe_s=0;
            signal(SIGCHLD,SIG_IGN); 
            // return;
            // wait(NULL);
        }
        else if(!is_end)// not the end of the job
        {
            close_pipe(left_pipe);
            user_pipe_r=0;
            user_pipe_s=0;
            signal(SIGCHLD,SIG_IGN);
            // wait(NULL);
        }
        // else if(user_pipe_r==1)
        // {
        //     string fifo_name="/user_pipe/fifo_"+to_string(id_r)+"_"+to_string(c_id);
        //     close()
        // }
        else//the last tiny job
        {
            //cout<<"elisa1"<<endl;

            if(i!=-1 || rec_number_pipe )//|| user_pipe_r==1)//the process receive some message from number pipe or pipe FIFO
            {
                //cout<<"elisa2"<<endl;
                close_pipe(left_pipe);
            }
            if (user_pipe_s==1)
            {
                //cout<<"user_pipe_s==1"<<endl;
                signal(SIGCHLD,SIG_IGN);
                return;
            }
            //cout<<"elisa3"<<endl;
            // user_pipe_r=0;
            // user_pipe_s=0;
            //cout<<"elisa4"<<endl;
            wait(&pid);
            //cout<<pid<<endl;
            //cout<<"elisa5"<<endl;
        }
        // wait(NULL);
    }
    return;

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
void who(int c_id)
{
    string client_mes="<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    for(int i=1;i<31;i++)
    {
        if(clients_arr[i].connfd!=-1)
        {
            client_mes+=(to_string(i)+"\t"+string(clients_arr[i].user_name)+"\t"+string(clients_arr[i].ip)+":"+to_string(clients_arr[i].port));
            if(i==c_id)
            {
                client_mes+="\t<-me\n";
            }
            else client_mes+="\n";
        }
    }
    // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);   
    send_mes(c_id,client_mes);        

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
        // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
        send_mes(c_id,client_mes);        


    }
    else
    {
        strcpy(clients_arr[c_id].user_name,name.c_str());
        string client_mes="*** User from "+string(clients_arr[c_id].ip)+":"+to_string(clients_arr[c_id].port)+" is named \'"+name+"\'. ***\n";
        send_to_all(client_mes);
    }
}
void logout(int i)
{
    string client_mes="*** User \'"+string(clients_arr[i].user_name)+"\' left. ***\n";
    for(int j=1;j<31;j++)
    {
        if(clients_arr[j].connfd!=-1 && i!=j)
        {
            // send(clients_arr[j].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
            send_mes(j,client_mes);        

        }
    }
    close(clients_arr[i].connfd);
    initial_client(i); 
    shmdt(clients_arr);
}
void yell(int c_id ,string mes)
{
    string client_mes="*** "+string(clients_arr[c_id].user_name)+" yelled ***: "+mes+"\n";
    send_to_all(client_mes);    
}
void tell(int c_id,int rec_id,string message)
{
    if(rec_id<31 && clients_arr[rec_id].connfd!=-1)
    {
        string client_mes="*** "+string(clients_arr[c_id].user_name)+" told you ***: "+message+"\n";
        // send(clients_arr[rec_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
        send_mes(rec_id,client_mes);        
    }
    else
    {
        string client_mes="*** Error: user #"+to_string(rec_id)+" does not exist yet. ***\n";
        // send(clients_arr[c_id].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
        send_mes(c_id,client_mes);        
    }
}

void processing_client(int c_id,char* buffer)
{
    queue<string> jobs_queue;
    string inp=remove_endl(buffer);
    split_job(jobs_queue,inp);
    while(!jobs_queue.empty())
    {
        jobs_id+=1;
        pair<int,int>left_pipe,right_pipe;
        string job_string=jobs_queue.front();
        jobs_queue.pop();
        vector<string> small_job_vec;
        small_job_vec=split_small_job(job_string);
        string small_job="";
        string next="";
        int user_pipe_s=0;
        int user_pipe_r=0;
        if( (pipe_map.find(jobs_id) != pipe_map.end()))//there exist some pipes that sent messages through number pipe to this process
        {
            for(int i=0;i<small_job_vec.size()-1;i++)
            {
                user_pipe_s=0;
                user_pipe_r=0;
                small_job=small_job_vec[i];
                next=small_job_vec[i+1];
                int id_s=-2;
                int id_r=-2;
                is_user_pipe(small_job,&id_s,&id_r);
                user_pipe_r=create_read_pipe(id_r,c_id,job_string);
                if(user_pipe_r==1)
                {
                    //cout<<"rec success"<<endl;
                    // left_pipe=clients_arr[c_id].user_pipes[id_r]; FIFO
                }
                user_pipe_s=create_send_pipe(id_s,c_id,job_string);
                if(user_pipe_s==1)
                {
                    //cout<<"send success"<<endl;
                    // right_pipe=clients_arr[id_s].user_pipes[c_id];FIFO
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
                    Setenv(const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                    // clients_arr[c_id].home=tiny_jobs[2];
                    // clients_arr[c_id].P=tiny_jobs[1];
                    continue;
                }
                else if(!is_number_pipe(next) && next=="|")	// stdout-> pipe
                {
                    pair<int,int>temp;
                    temp=create_pipe();
                    if(i==0)
                    {
                        left_pipe=pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,false,true,false,pipe_map);

                }
                else if(is_number_pipe(next)) //stdout->number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,pipe_map);
                    if(i==0)
                    {
                        left_pipe=pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,false,pipe_map);
            
                } 
                else if(next[0]=='!') //stdout -> number pipe and  stderr-> number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,pipe_map);
                    if(i==0)
                    {
                        left_pipe=pipe_map[jobs_id];
                    }
                    else
                    {
                        left_pipe=right_pipe;
                    }
                    right_pipe=temp;	
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,true,pipe_map);
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
                Setenv(const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                // clients_arr[c_id].home=tiny_jobs[2];
                // clients_arr[c_id].P=tiny_jobs[1];
                continue;
            }
            if(small_job=="exit")
            {
                logout(c_id);
                exit(0);
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
                left_pipe=pipe_map[jobs_id];
                i=-1;
            }
            int id_s=-2;
            int id_r=-2;
            is_user_pipe(small_job,&id_s,&id_r);
            user_pipe_s=create_send_pipe(id_s,c_id,job_string);
            if(user_pipe_s==1)
            {
                // right_pipe=clients_arr[id_s].user_pipes[c_id];FIFO
            }
            // else if(id_s!=-1)
            // {
            //     continue;
            // }
            job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,true,true,false,pipe_map);

        }
        else//none of pipes sent messages through number pipe to this process
        {
            for(int i=0;i<small_job_vec.size()-1;i++)
            {
                user_pipe_s=0;
                user_pipe_r=0;
                small_job=small_job_vec[i];
                next=small_job_vec[i+1];
                int id_s=-2;
                int id_r=-2;
                is_user_pipe(small_job,&id_s,&id_r);
                user_pipe_r=create_read_pipe(id_r,c_id,job_string);
                //cout<<"HIHI"<<endl;
                if(user_pipe_r==1)
                {
                    //cout<<"rec success"<<endl;
                    // left_pipe=clients_arr[c_id].user_pipes[id_r];FIFO
                    // clients_arr[c_id].user_pipes[id_r]=-1;
                }
                user_pipe_s=create_send_pipe(id_s,c_id,job_string);
                if(user_pipe_s==1)
                {
                    //cout<<"send success"<<endl;
                    // right_pipe=clients_arr[id_s].user_pipes[c_id];FIFO
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
                    Setenv(const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                    // clients_arr[c_id].home=tiny_jobs[2];
                    // clients_arr[c_id].P=tiny_jobs[1];
                    break;
                }
                if(tiny_jobs[0]=="tell" || tiny_jobs[0]=="yell" || tiny_jobs[0]=="name")
                {
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
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,false,false,false,pipe_map);
                }
                else if(is_number_pipe(next))//stdout -> number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_number_pipe(jobs_id+jump_n,pipe_map);
                    if(i!=0)
                    {
                        left_pipe=right_pipe;
                    }                        
                    right_pipe=temp;
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,false,pipe_map);
                }
                else if(next[0]=='!')//stdout->number pipe and stderr->number pipe
                {
                    const char* t=next.c_str();
                    int jump_n=atoi(t+1);
                    pair<int,int>temp;
                    temp=create_error_pipe(jobs_id+jump_n,pipe_map);
                    if(i!=0)
                    {
                        left_pipe=right_pipe;
                    }                        
                    right_pipe=temp;	
                    job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,true,pipe_map);
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
                Setenv(const_cast<char*>(tiny_jobs[1].c_str()),const_cast<char*>(tiny_jobs[2].c_str()));
                continue;
            }
            if(tiny_jobs[0]=="printenv")
            {
                if(findenv((char*)tiny_jobs[1].c_str())!=nullptr)
                {
                    send_mes(c_id,string(findenv((char*)tiny_jobs[1].c_str()))+"\n");
                }
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
                const char* t=(const char*)buffer;
                string mes=string(t+5);
                yell(c_id,mes);
                continue;
            }
            if(tiny_jobs[0]=="tell")
            {
                string mes="";
                int space = 2;
                string tell_mes=string(buffer);
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
                exit(0) ;
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
            int id_s=-2;
            int id_r=-2;
            is_user_pipe(small_job,&id_s,&id_r);
            user_pipe_r=create_read_pipe(id_r,c_id,job_string);
            if(user_pipe_r==1)
            {
                //cout<<"rec success"<<endl;
                // left_pipe=clients_arr[c_id].user_pipes[id_r];FIFO
                // clients_arr[c_id].user_pipes[id_r]=-1;
            }
            user_pipe_s=create_send_pipe(id_s,c_id,job_string);
            //cout<<"id =-2 user_pipe_s ";
            //cout<<user_pipe_s<<endl;
            if(user_pipe_s==1)
            {
                //cout<<"send success"<<endl;
                // right_pipe=clients_arr[id_s].user_pipes[c_id];FIFO
            }
            job_processing(user_pipe_r,user_pipe_s,c_id,clients_arr[c_id].connfd,i,jobs_id,0,small_job,left_pipe,right_pipe,true,false,false,pipe_map); 
        }
    }
    
}
void run(int i)
{
    string P = "PATH";
    string home = "bin:.";
    Setenv(const_cast<char*>(P.c_str()),const_cast<char*>(home.c_str()));
    greeting(clients_arr[i].connfd);
    login(i);
    string str = "% ";
    // //cout<<"??????????????????????????????????????????????????????????????????"<<endl;
    send_mes(i,str);
    // send(clients_arr[i].connfd, str.c_str(), strlen(str.c_str()), 0);
    while (1)
    {
        char buffer[15000];
        int readcount = read(clients_arr[i].connfd,buffer,15000);
        if(readcount == 0)  
        {
            string client_mes="*** User \'"+string(clients_arr[i].user_name)+"\' left. ***\n";
            for(int j=1;j<31;j++)
            {
                if(clients_arr[j].connfd!=-1 && i!=j)
                {
                    // send(clients_arr[j].connfd, client_mes.c_str(), strlen(client_mes.c_str()), 0);
                    send_mes(j,client_mes);        

                }
            }
            close(clients_arr[i].connfd);
            // client temp;
            // clients_arr[i]=temp;
            logout(i);
            exit(0);
        }
        else if(readcount <0)
        {
            continue;
        }
        else
        {
            processing_client(i,buffer);
            string str = "% ";
            // //cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
            send_mes(i,str);

            // send(clients_arr[i].connfd, str.c_str(), strlen(str.c_str()), 0);
        }
    }
}
int main(int argc, const char*argv[])
{ 
    mkdir("user_pipe", 0777);
    string P = "PATH";
    string home = "bin:.";
    Setenv(const_cast<char*>(P.c_str()),const_cast<char*>(home.c_str()));  
    int PORT=stoi(argv[1], nullptr);
    shm_clients_arr_id = shmget(IPC_PRIVATE, sizeof(client)*31, IPC_CREAT | 0666);
    clients_arr = (client *)shmat(shm_clients_arr_id, NULL, 0);
    for(int i=0;i<31;i++)
    {
        initial_client(i);
    }
    int listenfd=passiveTCP(argv[1],30);
    if(signal(SIGUSR1, mysignalHandler) == SIG_ERR) 
    { 
        perror("Can't set handler for SIGUSR1\n");
        exit(-1);
    }
    if(signal(SIGUSR2, mysignalHandler) == SIG_ERR) 
    { 
        perror("Can't set handler for SIGUSR1\n");
        exit(-1);
    }
    if(signal(SIGINT, mysignalHandler) == SIG_ERR) 
    { 
        perror("Can't set handler for SIGUSR1\n");
        exit(-1);
    }
    struct sockaddr_in servaddr,clitaddr;
    //cout<<"Waiting for connections ... "<<endl;
    while (1)
    {
        
        int connfd; 
        socklen_t addr_len = sizeof(clitaddr);
        connfd = accept(listenfd,(struct sockaddr *)&clitaddr,(socklen_t*)&addr_len);  
        if(connfd <0)
        {
            continue ;
        }
        // key_t key = ftok("/dev/shm/myshm", 0);
        bool have_sapce=false;
        int i=1;
        for(;i<31;i++)
        {
            if(clients_arr[i].id == -1)
            {
                have_sapce=true;
                clients_arr[i].id=i;
                clients_arr[i].connfd = connfd;
                clients_arr[i].port=ntohs(clitaddr.sin_port);
                strcpy(clients_arr[i].ip, inet_ntoa(clitaddr.sin_addr));
                // clients_arr[i].ip=string(inet_ntoa(clitaddr.sin_addr));
                break;
            }
        }        
        if(have_sapce==false)
        {
            close(connfd);
        }
        int status;
        pid_t p_id = fork();
        while (p_id < 0) 
        {
            wait(&status);
            p_id = fork();
        }
        if (p_id==0) 
        {
            clients_arr[i].p_id=getpid();
            client_id=i;
            run(i);
        }
        else if(p_id>0)//parent
        {
            close(connfd);
            signal(SIGCHLD,SIG_IGN);
            
        }
        
    }
    close(listenfd);
    return 0;

}
