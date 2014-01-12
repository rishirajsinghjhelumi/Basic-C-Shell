#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ptrace.h>
#include<sys/stat.h> 
#include<signal.h>
#include<time.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
//#include<wait.h>
#include<fcntl.h>
#define gc getchar_unlocked
#define MAX 256
struct process
{
	int proc_pid;
	int proc_run;
	char *proc_name;
	char *proc_args[MAX];
	int proc_num_args;
}proc[10000];
int num_proc = 0,pid_out;
void printer();
void execute_process(char *command,char *argm[],int num_args);
void execute_process_redirect(char *command,char *argm[],int num_args);
int execute_process_redirect_for_pipes(char *command,char *argm[],int num_args);
int execute_process_pipe(char *command,char *argm[],int num_args);
void execute_process_redirect(char *command,char *argm[],int num_args)
{
	int i,out,j=0,k,in,found=0;
	pid_t pid;
	char *new_command,*new_argm[MAX];
	for(i=0;i<num_args-1;i++)
		if( (!strcmp(argm[i],"<")) || (!strcmp(argm[i],">")) )
		{
			found = 1;
			break;
		}
	for(k=0;k<i;k++)
		new_argm[k] = strdup(argm[k]);
	new_argm[k] = NULL;
	new_command = strdup(command);
	if( !strcmp(argm[i],">") )
	{
		while(1)
		{
			if(argm[i] && !strcmp(argm[i],">"))
			{
				out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				i+=2;
			}
			else
			{
				i-=2;
				break;
			}
		}
		out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		dup2(out,1);
		close(out);
	}
	else if( !strcmp(argm[i],"<") )
	{
		in = open(argm[k+1], O_RDONLY);
		dup2(in,0);
		close(in);
		if(argm[k+2] && !strcmp(argm[k+2],">") )
		{
			i = k + 2;
			while(1)
			{
				if(argm[i] && !strcmp(argm[i],">"))
				{
					out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					i+=2;
				}
				else
				{
					i-=2;
					break;
				}
			}
			out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out,1);
			close(out);
		}
	}
	if(get_process(new_command,new_argm,k+1))
	{
		execvp(new_command,new_argm);
		if(errno==2)
			kill(getpid(),SIGKILL);
	}
	else
		exit(0);
}
int execute_process_redirect_for_pipes(char *command,char *argm[],int num_args)
{
	int i,out,j=0,k,in,found=0;
	pid_t pid;
	char *new_command,*new_argm[MAX];
	for(i=0;i<num_args-1;i++)
		if( (!strcmp(argm[i],"<")) || (!strcmp(argm[i],">")) )
		{
			found = 1;
			break;
		}
	if(!found)
		return 0;
	for(k=0;k<i;k++)
		new_argm[k] = strdup(argm[k]);
	new_argm[k] = NULL;
	new_command = strdup(command);
	if( !strcmp(argm[i],">") )
	{
		while(1)
		{
			if(argm[i] && !strcmp(argm[i],">"))
			{
				out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				i+=2;
			}
			else
			{
				i-=2;
				break;
			}
		}
		out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		dup2(out,1);
		close(out);
	}
	else if( !strcmp(argm[i],"<") )
	{
		in = open(argm[k+1], O_RDONLY);
		dup2(in,0);
		close(in);
		if(argm[k+2] && !strcmp(argm[k+2],">") )
		{
			i = k + 2;
			while(1)
			{
				if(argm[i] && !strcmp(argm[i],">"))
				{
					out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					i+=2;
				}
				else
				{
					i-=2;
					break;
				}
			}
			out = open(argm[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out,1);
			close(out);
		}
	}
	if(get_process(new_command,new_argm,k+1))
	{
		execvp(new_command,new_argm);
		if(errno==2)
			kill(getpid(),SIGKILL);
	}
	return 1;
}
int execute_process_pipe(char *command,char *argm[],int num_args)
{
	int i,j,k,pipe_count=0,status;
	for(i=0;i<num_args-1;i++)
		if(!strcmp(argm[i],"|"))
			pipe_count++;
	if(pipe_count)
	{
		int pipe_fd[pipe_count<<1],j=0,k=0,num_of_args[pipe_count+2];
		char *new_argm[pipe_count+2][MAX];
		for(i=0;i<num_args-1;i++)
		{
			if(!strcmp(argm[i],"|"))
			{
				new_argm[j][k] = NULL;
				num_of_args[j++] = k+1;
				k=0;
			}
			else
				new_argm[j][k++] = strdup(argm[i]);
		}
		new_argm[j][k] = NULL;
		num_of_args[j] = k+1;
		for(i=0;i<pipe_count;i++)
			pipe(pipe_fd+i*2);
		for(i=0;i<=j;i++)
		{
			pid_t pid;
			pid = fork();
			if(!pid)
			{
				if(i)
					dup2(pipe_fd[(i-1)<<1],0);
				if(i!=j)
					dup2(pipe_fd[(i<<1)+1],1);
				for(k=0;k<pipe_count<<1;k++)
					close(pipe_fd[k]);
				char *new_command = strdup(new_argm[i][0]);
				if(!execute_process_redirect_for_pipes(new_command,new_argm[i],num_of_args[i]))
				{
					if(get_process(new_command,new_argm[i],num_of_args[i]))
					{
						execvp(new_command,new_argm[i]);
						if(errno==2)
							kill(getpid(),SIGKILL);
					}
					else
						exit(0);
				}
			}
			else
				if(!i)
				{
					proc[num_proc].proc_pid = pid;
					num_proc++;
				}
		}
		for(i=0;i<pipe_count<<1;i++)
			close(pipe_fd[i]);
		for(i=0;i<=pipe_count;i++)
			wait(&status);
		return 1;
	}
	else
		return 0;
}
void sig_handler(int signum)
{
	if(signum == 2 || signum == 20 || signum==3)
		return;
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGTSTP, sig_handler);
	return;
}
void child_handler(int signum)
{
	int pid;
	pid = waitpid(-1, NULL, WNOHANG);
	if(pid != -1)
	{
		int p = check_process(pid),i;
		if(proc[p].proc_run)
		{
			printf("\n%s ",proc[p].proc_name);
			for(i=1;i<proc[p].proc_num_args-1;i++)
				printf("%s ",proc[p].proc_args[i]);
			printf("%d exited normally\n",proc[p].proc_pid);
			proc[p].proc_run = 0;
		}
	}
	signal(SIGCHLD, child_handler);
	return;
}
void print_last_process()
{
	printf("command %s ",proc[num_proc-1].proc_name);
	int i;
	for(i=1;i<proc[num_proc-1].proc_num_args-1;i++)
		printf("%s ",proc[num_proc-1].proc_args[i]);
	printf("pid %d\n",proc[num_proc-1].proc_pid);
}
int check_process(int pid)
{
	int i;
	for(i=0;i<num_proc;i++)
		if(proc[i].proc_pid==pid)
			return i;
}
int string_to_int(char *str)
{
	int x = 0,l = strlen(str),i;
	for(i=0;i<l;i++)
		x = (x<<1) + (x<<3) + str[i] - '0';
	return x;
}
void printer()
{
	char host_name[1024];
	host_name[1023] = '\0';
	gethostname(host_name, 1023);
	char *user_name = getenv("USER");
	char p_w_d[MAX];
	getcwd(p_w_d,sizeof(p_w_d));
	char *home_name = getenv("HOME");
	int i,l1 = strlen(home_name),check=1,l2=strlen(p_w_d);
	for(i=0;i<l1;i++)
		if(home_name[i]!=p_w_d[i])
		{
			check = 0;
			break;
		}
	if(check)
	{
		p_w_d[0] = '~';
		for(i=l1;i<l2;i++)
			p_w_d[i-l1+1] = p_w_d[i];
		p_w_d[l2-l1+1] = '\0';
	}
	printf("<%s@%s:%s>",user_name,host_name,p_w_d);
}
void change_dir(char *argm)
{
	char current[MAX],final[MAX],temp[MAX];
	getcwd(current,sizeof(current));
	if(!strcmp(argm,"."))
		return;
	else if( (!strcmp(argm,"..")) && !strcmp(current,"/"))
		return;
	else if(!strcmp(argm,"/"))
		chdir("/");
	else if(!strcmp(argm,".."))
	{
		int len = strlen(current),i;
		for(i=len-1;i>=0;i--)
			if(current[i]=='/')
				break;
		current[i] = '\0';
		if(i==0)
			strcpy(current,"/");
		chdir(current);
	}
	else
	{
		if(argm[0] != '/' || argm[0] != '.')
		{
			strcat(current,"/");
			strcat(current,argm);
			chdir(current);
		}
		else if(argm[0] == '/')
			chdir(argm);
		else
		{
			int len = strlen(current),i;
			for(i=0;i<len;i++)
				if(current[i]=='/')
					break;
			change_dir(strndup(current,i));
			change_dir(&current[i]);
		}
	}
}
void ls_out()
{
	DIR* a = opendir(".");
	struct dirent *b;
	while((b=readdir(a)))
	{
		if(b->d_name[0] != '.')
			printf("%s\n", b->d_name);
	}
	//printf("\n");
}
void hist_all()
{
	int i,j;
	for(i=0;i<num_proc;i++)
	{
		printf("%d. %s ",i+1,proc[i].proc_name);
		for(j=1;j<proc[i].proc_num_args-1;j++)
			printf("%s ",proc[i].proc_args[j]);
		printf("\n");
	}
}
void hist_n(int n)
{
	int i,last=num_proc-n,j;
	last = (last<0)?0:last;
	for(i=last;i<num_proc;i++)
	{
		printf("%d. %s ",i-last+1,proc[i].proc_name);
		for(j=1;j<proc[i].proc_num_args-1;j++)
			printf("%s ",proc[i].proc_args[j]);
		printf("\n");
	}
}
void not_hist_n(int n)
{
	if(n>num_proc-1)
		return;
	char *c = strdup(proc[n-1].proc_name),*c1[MAX];
	int i;
	for(i=0;i<proc[n-1].proc_num_args-1;i++)
		c1[i] = strdup(proc[n-1].proc_args[i]);
	execute_process(c,c1,proc[n-1].proc_num_args);
}
void print_process(int n)
{
	printf("command name: %s ",proc[n].proc_name);
	int i;
	for(i=1;i<proc[n].proc_num_args-1;i++)
		printf("%s ",proc[n].proc_args[i]);
	printf("process id: %d\n",proc[n].proc_pid);
}
void pid_all()
{
	printf("List of all processes spawned from this shell:\n");
	int i;
	for(i=0;i<num_proc;i++)
		print_process(i);
}
void pid_current()
{
	printf("List of currently executing processes spawned from this shell:\n");
	int i;
	for(i=0;i<num_proc;i++)
		if(proc[i].proc_run)
			print_process(i);
}
int get_process(char *command,char *argm[],int num_args)
{
	int check = 1;
	if(num_args>2)
	{
		if(!strcmp(command,"pid"))
		{
			if(!strcmp(argm[1],"current"))
				pid_current();
			else if(!strcmp(argm[1],"all"))
				pid_all();
			check = 0;
			proc[num_proc].proc_run = 0;
			proc[num_proc].proc_pid = pid_out;
			num_proc++;
		}
		if(!strcmp(command,"cd"))
		{
			if(!strcmp(argm[1],getenv("HOME")))
				chdir(getenv("HOME"));
			else
				change_dir(argm[1]);

			check = 0;
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
		}
	}
	else
	{
		if(!strcmp(command,"pid"))
		{
			printf("command name: ./a.out process id: %d\n",pid_out);
			check = 0;
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
		}
		else if(!strcmp(command,"hist"))
		{
			hist_all();
			check = 0;
			proc[num_proc].proc_run = 0;
			proc[num_proc].proc_pid = pid_out;
			num_proc++;
		}
		else if(!strcmp(strndup(command,4),"hist") && strlen(command)>4)
		{
			hist_n(string_to_int(&command[4]));
			check = 0;
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
		}
		else if(!strcmp(strndup(command,5),"!hist"))
		{
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
			not_hist_n(string_to_int(&command[5]));
			check = 0;
		}
		else if(!strcmp(command,"quit"))
			exit(0);
		else if(!strcmp(command,"ls"))
		{
			ls_out();
			check = 0;
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
		}
		else if(!strcmp(command,"cd"))
		{
			char *home = getenv("HOME");
			chdir(home);
			check = 0;
			proc[num_proc].proc_pid = pid_out;
			proc[num_proc].proc_run = 0;
			num_proc++;
		}
	}
	return check;
}
void execute_process(char *command,char *argm[],int num_args)
{
	int bg = 0,i=0;
	pid_t pid;
	proc[num_proc].proc_run = 1;
	proc[num_proc].proc_name = strdup(command);
	proc[num_proc].proc_num_args = num_args;
	for(i=0;i<num_args-1;i++)
		proc[num_proc].proc_args[i] = strdup(argm[i]);
	int prev_num_proc = num_proc;
	int check1 = 0;
	for(i=0;i<num_args-1;i++)
		if(!strcmp(argm[i],"<") || !strcmp(argm[i],">"))
		{
			check1 = 1;
			break;
		}
	if(execute_process_pipe(command,argm,num_args))
	{
		proc[prev_num_proc].proc_run = 0;
		num_proc = prev_num_proc + 1;
	}
	else if(check1)
	{
		pid = fork();
		if(!pid)
			execute_process_redirect(command,argm,num_args);
		else
		{
			wait();
			proc[prev_num_proc].proc_run = 0;
			proc[prev_num_proc].proc_pid = pid;
			num_proc = prev_num_proc + 1;
		}
	}
	else 
	{
		if(get_process(command,argm,num_args))
		{
			if(num_args>2)
			{
				if(!strcmp(argm[num_args-2],"&"))
					bg = 1;
				else
					proc[num_proc].proc_run = 0;
				if((pid=fork())==0)
				{
					execvp(command,argm);
					if(errno==2)
						kill(getpid(),SIGKILL);
				}
				else
				{
					proc[num_proc].proc_pid = pid;
					if(!bg)
						wait();
					num_proc++;
				}
			}
			else
			{
				if(command[strlen(command)-1]=='&')
				{
					bg = 1;
					command = strndup(command,strlen(command)-1);
				}
				else
					proc[num_proc].proc_run = 0;
				if((pid=fork())==0)
				{
					execlp(command,"NONE",NULL);
					if(errno==2)
						kill(getpid(),SIGKILL);
				}
				else
				{
					proc[num_proc].proc_pid = pid;
					if(!bg)
						wait();
					num_proc++;
				}
			}
			if(bg)
				print_last_process();
		}
	}
}
void scanner()
{
	int i,num_args=0;
	char *command,*argm[MAX]={NULL},str[MAX],ch = gc();
	if(ch!=-1 && ch!=3)
	{
		while(ch != '\n')
		{
			while(ch == ' ' || ch == '\t')
				ch = gc();
			int j=0;
			if(ch=='\n')
				break;
			while(1)
			{
				if( ch == ' ' || ch == '\n' || ch == '\t')
					break;
				str[j++] = ch;
				ch = gc();
			}
			str[j] = '\0';
			if(!strcmp("~",str))
				strcpy(str,getenv("HOME"));
			argm[num_args++] = strdup(str);
		}
		argm[num_args++] = NULL;
		if(argm[0])
		{
			command = strdup(argm[0]);
			execute_process(command,argm,num_args);
		}
	}
	else
		printf("\n");
}
int main()
{
	signal(SIGINT,  sig_handler);
	signal(SIGTSTP, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGCHLD, child_handler);
	pid_out = getpid();
	while(1)
	{
		printer();
		fflush(stdout);
		fflush(stdin);
		scanner();
		fflush(stdout);
		fflush(stdin);
	}
	return 0;
}
