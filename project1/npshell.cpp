
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
using namespace std;
char* findenv(char* name)
{
    char* value = getenv(const_cast<char*>(name));
    return value;
}
int Setenv(char* name,char* value)
{
    int ret = setenv(name,value,1);
    return 0;
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
vector<string> string_processing(string input,string &f_name,bool &redirection2)
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
        jobs_queue.push(job_string);

    }  
    // while (!jobs_queue.empty())
    // {
    //     cout<<jobs_queue.front()<<endl;
    //     jobs_queue.pop();
    // }
    
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
        // cout << "pipe faild" << endl;
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
    // for(int i=0;i<small_job_vec.size();i++)
    // {
    //     cout<<"!!!!!"<<small_job_vec[i]<<endl;
    // }
    return small_job_vec;

}
void processing_small_job(string small_job)
{
    string f_name;
    bool redirection2=false;
    vector<string> small_job_vec= string_processing(small_job,f_name,redirection2);
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
        if (execvp(args[0], args) != -1)
            ;
        else
        {
            cerr<< "Unknown command: ["  << args[0] <<"]."<< endl;
            exit(0);
        }			
    }
    else
    {
        if (execvp(args[0], args) != -1)
            ;
        else
		{
            cerr<< "Unknown command: ["  << args[0] <<"]."<< endl;
        	exit(0);  
		}
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
void job_processing(int i,int job_id,int jump_n ,string small_job,pair<int,int>left_pipe,pair<int,int>right_pipe,bool is_end,bool rec_number_pipe,bool is_error_pipe,map<int,pair<int,int>>&pipe_map)
{
    pid_t pid ;
    pipe_map.erase(job_id);
    while(1)
    {
        pid=fork();
        if(pid>=0)
        {
            break;
        }
    }
    // if(pid<0)
    // {
    //     cout << "fork failed" << endl;
    //     exit(-1); 
    // }
    if(pid==0)
    { 
        close_number_pipes(pipe_map,job_id,jump_n);
        if(rec_number_pipe)//there exist some pipes that sent messages through number pipe to this process
        {
            if(i==-1)// only one tiny job (e.g ls)
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
            }
            else if(is_end) // the last tiny job
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
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

        }
        else//none of pipes sent messages through number pipe to this process
        {
            if(is_end)//the last tiny job
            {
                dup2(left_pipe.first,STDIN_FILENO);
                close_pipe(left_pipe);
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
        }
        processing_small_job(small_job);                            
    }
    else//parent
    {
        if(!rec_number_pipe && i==0)//the process didn't receive message form number pipe or pipe and isn't the end of the job
        {
            signal(SIGCHLD,SIG_IGN); 
            return;
            // wait(NULL);
        }
        else if(!is_end)// not the end of the job
        {
            close_pipe(left_pipe);
            signal(SIGCHLD,SIG_IGN);
            // wait(NULL);
        }
        else//the last tiny job
        {
            if(i!=-1 || rec_number_pipe)//the process receive some message from number pipe or pipe
            {
                close_pipe(left_pipe);
            }
            wait(NULL);
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

int main()
{   
    queue<string> jobs_queue;
    map<int,pair<int,int>>pipe_map;
    string P = "PATH";
    string home = "bin:.";
    int a=Setenv(const_cast<char*>(P.c_str()),const_cast<char*>(home.c_str()));
    int jobs_id=0;
	while(1)
    {
        cout << "% ";
        string inp;
		getline(cin, inp);
        if(cin.eof())
        {
            return 0;
        }
        split_job(jobs_queue,inp);
        while(!jobs_queue.empty())
        {
            jobs_id+=1;
            pair<int,int>left_pipe,right_pipe;
            string job_string=jobs_queue.front();
            jobs_queue.pop();
            // cout<<"########"<<job_string<<"  "<<jobs_id<<endl<<endl;
            vector<string> small_job_vec;
            small_job_vec=split_small_job(job_string);
            string small_job="";
            string next="";
            if( (pipe_map.find(jobs_id) != pipe_map.end()))//there exist some pipes that sent messages through number pipe to this process
            {
                for(int i=0;i<small_job_vec.size()-1;i++)
                {
                    small_job=small_job_vec[i];
                    next=small_job_vec[i+1];
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
                    }
                    else if(!is_number_pipe(next) && next=="|")	// stdout-> pipe
                    {//
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
                        // cout<<" L "<<	left_pipe.first<<left_pipe.second<<" R ";	
                        // cout<<	right_pipe.first<<right_pipe.second<<endl;
                        job_processing(i,jobs_id,0,small_job,left_pipe,right_pipe,false,true,false,pipe_map);

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
                        // cout<<"jobid :" <<jobs_id<<" jj "<<jobs_id+jump_n<<" L ";
                        // cout<<	left_pipe.first<<left_pipe.second<<" R ";	
                        // cout<<	right_pipe.first<<right_pipe.second<<endl;
                        job_processing(i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,false,pipe_map);
				
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
                        job_processing(i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,true,true,pipe_map);
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
                }
                if(small_job=="exit")
                {
                    return 0;
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
                // cout<<small_job;
                // cout<<" jobid :" <<jobs_id<<" jj "<<" L ";
                // cout<<	left_pipe.first<<left_pipe.second<<" R ";	
                // cout<<	right_pipe.first<<right_pipe.second<<endl;
                job_processing(i,jobs_id,0,small_job,left_pipe,right_pipe,true,true,false,pipe_map);

            }
			else//none of pipes sent messages through number pipe to this process
			{
                for(int i=0;i<small_job_vec.size()-1;i++)
                {
                    small_job=small_job_vec[i];
                    next=small_job_vec[i+1];
                    // cout<<small_job<<endl;
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
                        // cout<<" L "<<	left_pipe.first<<left_pipe.second<<" R ";	
                        // cout<<	right_pipe.first<<right_pipe.second<<endl;
                        job_processing(i,jobs_id,0,small_job,left_pipe,right_pipe,false,false,false,pipe_map);
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
                        // cout<<"jobid :" <<jobs_id<<" jj "<<jobs_id+jump_n;
                        // cout<<" L "<<	left_pipe.first<<left_pipe.second<<" R ";	
                        // cout<<	right_pipe.first<<right_pipe.second<<endl;
                        job_processing(i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,false,pipe_map);
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
                        job_processing(i,jobs_id,jump_n,small_job,left_pipe,right_pipe,false,false,true,pipe_map);
                    }
	
				}                
				left_pipe.first=right_pipe.first;
				left_pipe.second=right_pipe.second;
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
                }
                if(small_job=="exit")
                {
                    return 0;
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
                    i=-2;
                }
                // cout<<small_job;
                // cout<<" jobid :" <<jobs_id<<" jj "<<" L ";
                // cout<<	left_pipe.first<<left_pipe.second<<" R ";	
                // cout<<	right_pipe.first<<right_pipe.second<<endl;
                job_processing(i,jobs_id,0,small_job,left_pipe,right_pipe,true,false,false,pipe_map); 
			}
        }
    }
}
