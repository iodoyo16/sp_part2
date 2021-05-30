/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void recursive_pipe(char* command[][CMD_LEN],int fd[], int cmd_idx);
int pipe_execute(char *argv[],int bg,char* cmdline);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	    printf("CSE4100-SP-P4> ");                   
	    Fgets(cmdline, MAXLINE, stdin); 
	    if (feof(stdin))
	        exit(0);

	/* Evaluate */
	    eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    int argc=0;////////////////////////////////////////
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        pipe_execute(argv,bg,cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0);  
    if (!strcmp(argv[0], "exit")) /* quit command */
	    exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    if (!strcmp(argv[0], "cd")){
        if(argv[1]==NULL||(strcmp(argv[1],"~")==0)||(strcmp(argv[1],"$HOME")==0)){
            if(chdir(getenv("HOME"))<0)
                printf("cd error");
        }
        else if(chdir(argv[1])<0){
            printf("no such directory\n");
        }
        return 1;
    }          
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */
    int quoteflag=0;

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    if(*buf=='\''){
        ++buf;
        delim=strchr(buf,'\'');
        quoteflag=1;
    }
    else if(*buf=='\"'){
        ++buf;
        delim=strchr(buf,'\"');
        quoteflag=2;
    }
    else
        delim=strchr(buf,' ');
    while (delim!=NULL) {
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
        if(*buf=='\''){
            quoteflag=1;
            *buf='\0';
            buf++;
            delim=strchr(buf,'\'');
        }
        else if(*buf=='\"'){
            quoteflag=2;
            *buf='\0';
            buf++;
            delim=strchr(buf,'\"');
        }
        else{
            delim=strchr(buf,' ');
        }
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	    argv[--argc] = NULL;
    return bg;
}
/* $end parseline */

/* recursive pipe function*/
int pipe_execute(char *argv[],int bg,char* cmdline){
    char* command[MAXARGS][CMD_LEN];
    int fd[2];
    int cmd_cnt=0;
    int splited_argc=0;
    pid_t pid_first,pid_second;
    char pathname[20]="/bin/";
    for(int i=0;argv[i]!=NULL;i++){
        if(!strcmp(argv[i],"|")){
            command[cmd_cnt][splited_argc]=NULL;
            cmd_cnt++;
            splited_argc=0;
        }
        else{           
            command[cmd_cnt][splited_argc]=argv[i];
            splited_argc++;
        }
    }
    command[cmd_cnt][splited_argc]=NULL;
    command[++cmd_cnt][0]=NULL;
    strcat(pathname,command[cmd_cnt-1][0]);
    if(cmd_cnt==1){
        pid_t pid;
        if((pid=Fork())==0){                            // Child
            if (execve(pathname, argv, environ) < 0) {	// ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
	    if (!bg){ 
	        int status;
            Waitpid(pid,&status,0);
	    }
        else{
            printf("%d %s",pid,cmdline);
        }
        return 0;
    }
    if((pid_first=Fork())>0){ // parent
        int status;
        Wait(&status);
        return status;
    }


    if(pipe(fd)<0){
        perror("pipe error\n");
        exit(1);
    }
    if((pid_second=Fork())==0){
       recursive_pipe(command,fd,cmd_cnt-2);
    }
    Dup2(fd[0],STDIN_FILENO);
    Close(fd[0]);
    Close(fd[1]);
    if(execvp(command[cmd_cnt-1][0],command[cmd_cnt-1])<0)
        unix_error("execvp error");
    return 0;
}
/* recursive pipe */
void recursive_pipe(char* command[][CMD_LEN],int fd[], int cmd_idx){
    int curfp[2];
    pid_t pid;
    if((cmd_idx)==0){
        Dup2(fd[1],STDOUT_FILENO);
        Close(fd[1]);
        Close(fd[0]);
        if(execvp(command[cmd_idx][0],command[cmd_idx])<0)
            unix_error("execvp error");
    }
    else{
        if(pipe(curfp)<0){
            unix_error("pipe error");
        }
        if((pid=Fork())==0){
            --cmd_idx;
            recursive_pipe(command,curfp,cmd_idx);
        }
        Dup2(curfp[0],STDIN_FILENO);
        Close(curfp[0]);
        Close(curfp[1]);

        Dup2(fd[1],STDOUT_FILENO);
        Close(fd[1]);
        Close(fd[0]);
        if(execvp(command[cmd_idx][0],command[cmd_idx])<0)
            unix_error("execvp error");
    }
}