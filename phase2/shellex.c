/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void recursive_pipe(char* command[][10],int fd[], int cmd_cnt,int depth);
int pipe_execute(char *argv[],int bg);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	    printf("CSE4100:P4-myshell> ");                   
	    fgets(cmdline, MAXLINE, stdin); 
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
        pipe_execute(argv,bg);
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
        if(chdir(argv[1])<0){
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

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
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
int pipe_execute(char *argv[],int bg){
    char* command[MAXARGS][10];
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
            printf("you can execute process in background in phase 3");
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
       recursive_pipe(command,fd,cmd_cnt-1,1);
    }
    Close(STDIN_FILENO);
    Dup2(fd[0],STDIN_FILENO);
    Close(fd[0]);
    Close(fd[1]);
    if(execvp(command[cmd_cnt-1][0],command[cmd_cnt-1])<0)
        unix_error("execvp error");
    return 0;
}
/* recursive pipe */
void recursive_pipe(char* command[][10],int fd[], int pipe_cnt,int depth){
    int curfp[2];
    pid_t pid;
    if((pipe_cnt-depth)==0){
        Close(STDOUT_FILENO);
        Dup2(fd[1],STDOUT_FILENO);
        Close(fd[1]);
        Close(fd[0]);
        if(execvp(command[pipe_cnt-depth][0],command[pipe_cnt-depth])<0)
            unix_error("execvp error");
    }
    else{
        if(pipe(curfp)<0){
            perror("pipe error\n");
            exit(1);
        }
        if((pid=Fork())==0){
            ++depth;
            recursive_pipe(command,curfp,pipe_cnt,depth);
        }
        Close(STDIN_FILENO);
        Dup2(curfp[0],STDIN_FILENO);
        Close(curfp[0]);
        Close(curfp[1]);

        Close(STDOUT_FILENO);
        Dup2(fd[1],STDOUT_FILENO);
        Close(fd[1]);
        Close(fd[0]);
        if(execvp(command[pipe_cnt-depth][0],command[pipe_cnt-depth])<0)
            unix_error("execvp error");
    }
}