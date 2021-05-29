/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

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
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    /*
    if(*argv[i]=='\''&&*(argv[i]+strlen(argv[i])-1)=='\''){
        *(argv[i]+strlen(argv[i])-1)='\0';
        *argv[i]='\0';
        argv[i]++;
    }
    else if(*argv[i]=='\"'&&*(argv[i]+strlen(argv[i])-1)=='\"'){
        *(argv[i]+strlen(argv[i])-1)='\0';
        *argv[i]='\0';
        argv[i]++;
    }*/
    //for(int i=0;argv[i]!=NULL; i++)
    //    printf("%s\n",argv[i]);
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        char pathname[MAXLINE]="/bin/";
        strcat(pathname,argv[0]);
        if((pid=Fork())==0){                            // Child
            if (execve(pathname, argv, environ) < 0) {	// ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
	    /* Parent waits for foreground job to terminate */
	    if (!bg){ 
	        int status;
            Waitpid(pid,&status,0);
	    }
	    else//when there is backgrount process!
	        printf("%d %s", pid, cmdline);
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
    char *temp;
    int argc;            /* Number of args */
    int bg;              /* Background job? */
    int quoteflag=0;

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))||quoteflag) {////////////////////////
    ////////////////////////////////////////
        if(quoteflag!=0){
            if(quoteflag==1)
                temp=strchr(buf,'\'');
            else if(quoteflag==2)
                temp=strchr(buf,'\"');
            if(temp!=NULL)
                delim=temp;
            quoteflag=0;            
        }
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
        /////////////////////////////////////
        if(*buf=='\''){
            quoteflag=1;
            *buf='\0';
            buf++;
        }
        else if(*buf=='\"'){
            quoteflag=2;
            *buf='\0';
            buf++;
        }
        //////////////////////////
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


