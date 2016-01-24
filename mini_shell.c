#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#define MAX 256
int flag=0;
int ret;
int top=0;

struct bckgrd		//a structure that would store the pid and the command name that has been put into background
{
	pid_t pid;
	char cmd[200];
};
struct bckgrd arr[MAX];

char command[MAX];

void sigchld_handler(int signum)	//SIGCHLD handler that would remove a background job from the array once it has been completed
{
	pid_t pid_pro ;
	while((pid_pro = waitpid(-1,NULL,WNOHANG)) >0)
	{	
		int i;
		for(i=0;i<MAX;i++)
		{
			if(arr[i].pid == pid_pro)
			{
				arr[i].pid = -1;	
			}
		}
	}
}

void sigstop_handler(int signum) //is ctrl+z is pressed during a process, this handler will add it to the list of backgraound jobs
{
	int i;
	for(i=0;i<MAX;i++)
	{
		if(arr[i].pid==-1)
		{
			if(i>top){top=i;}
			arr[i].pid= ret;
	   		strcpy(arr[i].cmd,command);
			break;
		}
	}
}

void addtojob(pid_t pid)
{
	int i;
	for(i=0;i<MAX;i++)
	{
		if(arr[i].pid==-1)
		{
			if(i>top){top=i;}
			arr[i].pid= pid;
			strcpy(arr[i].cmd,command);
			break;
		}
	}
}

int main(){
	//initialising the array with pid=-1
	int k;
	for(k=0;k<MAX;k++)arr[k].pid =-1;
	//signal to call signal handlers for SIGCHLD and SIGTTOU
	signal(SIGCHLD,sigchld_handler);
	signal(SIGTTOU, SIG_IGN);
	ret =getpid();
	
	//blocking Ctrl+z ,Ctrl+C, Ctrl+\ in parent process
	sigset_t signal_set1,signal_set2,waiting_mask,origset;
	sigemptyset(&signal_set1);
	sigemptyset(&signal_set2);
	//sigaddset(&signal_set1,SIGINT);
	sigaddset(&signal_set2,SIGTSTP);
	sigaddset(&signal_set1,SIGQUIT);
	sigprocmask(SIG_BLOCK,&signal_set1,NULL);
	sigprocmask(SIG_BLOCK,&signal_set2,NULL);
	
	while(1)
	{
		printf("prompt>>");
		fflush(stdin);
		if(fgets(command,MAX,stdin)==NULL)		//takes input from terminal
			break;
		if(command[0]=='j' && command[1]=='o' && command[2]=='b' && command[3]=='s') //lists all the jobs
		{
			int i;
			printf("PID\tCOMMAND\n");
			for(i=0;i<=top;i++)
			{
				if(arr[i].pid!=-1)
				{
					printf("%d\t%s\n",arr[i].pid,arr[i].cmd);
				}
			}
		}
		else if(command[0]=='s' && command[1] == 't' && command[2] == 'a' && command[3]=='r' && command[4] =='t') //will start the background process
		{
			pid_t pid = atoi(command+6);
			kill(pid,SIGCONT);
		}
		else if(command[0]=='s' && command[1] == 't' && command[2] == 'o' && command[3]=='p' ) // will stop the background process
		{
			pid_t pid = atoi(command+5);
			kill(pid,SIGSTOP);
		}
		else{ 
			signal(SIGSTOP,sigstop_handler);
			if(command[strlen(command)-2]=='&')
			{
				command[strlen(command)-2]='\0';
				flag=1;  //implies that the command gien has an ampersand
			}
			//separating commands into list of arguments to pass to execvp
			char* argument[MAX];
			int argcount=0;
			int k;
			for(k=0;k<MAX;k++)
			{
				while(command[k]==' ')k++;
				if(command[k]=='\0') break;
				else
				{
					int temp=k;
					while(command[k]!=' ' && command[k]!='\n' && command[k]!='\0' )k++;
					command[k]='\0';
					argument[argcount] = (char*) malloc(sizeof(char)*MAX);
					strcpy(argument[argcount],command+temp);
					argcount++;				
				}
			}
			argument[argcount]=(char*)NULL;
			if(flag==1) //if there is & in the command(if the command is a background command)
			{
				ret=fork();
				if(ret==0)
				{
					sigset_t emptyset;
					sigemptyset(&emptyset);
					if(sigprocmask(SIG_SETMASK,&emptyset,&origset)==-1) ;
					setpgid(getpid(),getpid()); 
					//the process is not brought to the foreground s it is a background process
					execvp(argument[0],argument);
				}
				else
				{
					//in th parent add the job to the list of background jobs
					addtojob(ret);
				}
			}
			else //if there is no & in the command
			{
				ret=fork();
				if(ret==0)
				{
					sigset_t emptyset;
					sigemptyset(&emptyset);
					if(sigprocmask(SIG_SETMASK,&emptyset,&origset)==-1) perror("sigprocmask");
					signal(SIGTTOU, SIG_IGN);
					setpgid(getpid(),getpid());
					tcsetpgrp(0,getpid()); //bring the process to the foreground
					execvp(argument[0],argument);
					perror("Cannot execute execv\n");
				}
				else
				{
					int status;
					waitpid(ret,&status,WUNTRACED); //wait for the child until is not finished ot stopped
					if(WIFSTOPPED(status)) //if the child has been stopped using Ctrl+Z add it to the list of background jobs
					{
						//adding to the background jobs
						addtojob(ret);
					}
					tcsetpgrp(0,getpid()); //control of the terminal given to the paret once the child has executed or has been stopped
				}
			}	
		}
	}
	return 0;
}