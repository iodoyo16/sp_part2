/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128
#define MAXJOB 30

typedef struct _myjob{
    int job_id;
    pid_t pid;
    char state;
    char cmdline[MAXLINE];
}myjob;

myjob job_list[MAXJOB];
int job_cnt=1;
pid_t shell_pid;

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void split_cmd_by_pipe(char* argv[],char* command[][CMD_LEN],int* cmd_cnt);
void recursive_pipe(char* command[][CMD_LEN],int fd[], int cmd_idx);
void pipe_execute(char *argv[], int bg, char* cmdline,char* command[][CMD_LEN],int cmd_cnt);
void parent_shell_execute(int bg, pid_t pid ,char* cmdline);

void myjobs();
int getjid_by_pid(pid_t pid);
int getjob_by_pid(pid_t pid);
int deletejob_by_pid(pid_t pid);
int deletejob_by_jobid(int job_id);
int get_nextjid();
int addjob(char* cmdline, pid_t pid, int state);
pid_t get_fgpid();
void init_job_list();

void child_handler(int sig);
void int_handler(int sig);
void tstp_handler(int sig);
void child_handler(int sig);


int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    
    shell_pid=getpid();
    Signal(SIGCHLD,child_handler);
    Signal(SIGTSTP,tstp_handler);
    Signal(SIGINT,int_handler);
    init_job_list();
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
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        char* command[MAXARGS][CMD_LEN];
        int cmd_cnt=0;
        split_cmd_by_pipe(argv,command,&cmd_cnt);
        pipe_execute(argv,bg,cmdline,command,cmd_cnt);
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
    if(!strcmp(argv[0], "jobs")){
        myjobs();
        return 1;
    }
    /*
    if(!strcmp(argv[0], "bg")){
        return 1;
    }
    if(!strcmp(argv[0], "fg")){
        return 1;
    }
    if(!strcmp(argv[0], "kill")){
        return 1;
    }*/
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

    /* Build the argv list *////////////////////// "" must be solved//////////////////////////////////////////////////////////
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

void split_cmd_by_pipe(char* argv[] ,char* command[][CMD_LEN],int* cmd_cnt){
    int splited_argc=0;
    for(int i=0;argv[i]!=NULL;i++){
        if(!strcmp(argv[i],"|")){
            command[*cmd_cnt][splited_argc]=NULL;
            (*cmd_cnt)++;
            splited_argc=0;
        }
        else{
            command[*cmd_cnt][splited_argc]=argv[i];
            splited_argc++;
        }
    }
    command[*cmd_cnt][splited_argc]=NULL;
    command[++(*cmd_cnt)][0]=NULL;
}

/* recursive pipe function*/
void pipe_execute(char *argv[],int bg,char* cmdline,char* command[][CMD_LEN],int cmd_cnt){
    
    int fd[2];
    pid_t pid_first,pid_second;
    char pathname[20]="/bin/";

    sigset_t mask_one, prev_mask,mask_all;
    Sigfillset(&mask_all);
    Sigemptyset(&prev_mask);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one,SIGCHLD);
    /////////////////////////////////////
    for( int i=0;i<cmd_cnt;i++){
        for( int j=0; command[i][j]!=NULL; j++)
            printf("%s ", command[i][j]);
        printf("\n");
    }
    ////////////////////
    strcat(pathname,command[cmd_cnt-1][0]);
    if(cmd_cnt==1){
        pid_t pid;
        Sigprocmask(SIG_BLOCK,&mask_one,&prev_mask);
        if((pid=Fork())==0){                            // Child
            Sigprocmask(SIG_SETMASK,&prev_mask,NULL);
            Setpgid(0,0);///////////////////////
            if (execve(pathname, argv, environ) < 0) {	// ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        parent_shell_execute(bg,pid,cmdline);
        return;
    }

    if((pid_first=Fork())>0){ // parent
        parent_shell_execute(bg,pid_first,cmdline);
        return;
    }
    Sigprocmask(SIG_SETMASK,&prev_mask,NULL);
    Setpgid(0,0);///////////////////////
    if(pipe(fd)<0){
        unix_error("pipe error");
    }
    if((pid_second=Fork())==0){
       recursive_pipe(command,fd,cmd_cnt-2);
    }
    Close(STDIN_FILENO);
    Dup2(fd[0],STDIN_FILENO);
    Close(fd[0]);
    Close(fd[1]);
    if(execvp(command[cmd_cnt-1][0],command[cmd_cnt-1])<0)
        unix_error("execvp error");
    return ;
}
/* recursive pipe */
void recursive_pipe(char* command[][CMD_LEN],int fd[], int cmd_idx){
    int curfp[2];
    pid_t pid;
    if((cmd_idx)==0){
        Close(STDOUT_FILENO);
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
        Close(STDIN_FILENO);
        Dup2(curfp[0],STDIN_FILENO);
        Close(curfp[0]);
        Close(curfp[1]);

        Close(STDOUT_FILENO);
        Dup2(fd[1],STDOUT_FILENO);
        Close(fd[1]);
        Close(fd[0]);
        if(execvp(command[cmd_idx][0],command[cmd_idx])<0)
            unix_error("execvp error");
    }
}

void parent_shell_execute(int bg, pid_t pid ,char* cmdline){
    char state;
    if(!bg)state='F';
    else state='B';
    sigset_t mask_one, prev_mask,mask_all;

    Sigfillset(&mask_all);
    Sigemptyset(&prev_mask);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one,SIGCHLD);

    Sigprocmask(SIG_BLOCK,&mask_all,NULL);
    addjob(cmdline,pid,state);
    Sigprocmask(SIG_SETMASK,&prev_mask,NULL);
    int job_id=getjid_by_pid(pid);
    if(!bg){
        if(job_id<0)//already completed, reaped
            return ;
        while(job_list[job_id].pid==pid&&job_list[job_id].state=='F'){
            Sigsuspend(&prev_mask);
            printf("jid %d pid %d\n",job_id, (int)pid);
        }
        printf("suspend out\n");
    }
    else{
        printf("[%d] %d %s",job_id, pid, cmdline);
    }
    return ;
}
void int_handler(int sig){
    pid_t pid;
    int preverrono=errno;
    pid=get_fgpid();
    Sio_puts("\nfgpid: ");
    Sio_putl((long)pid);
    Sio_puts("\n");
    if(pid<=0){
        errno=preverrono;
        return;
    }
    if(kill(-pid,sig)<0)
        Sio_puts("\nSIGINT kill error\n");
    errno=preverrono;
    return;
}
void tstp_handler(int sig){
    int preverrono=errno;
    pid_t pid;
    pid=get_fgpid();
    Sio_puts("\nfgpid: ");
    Sio_putl((long)pid);
    Sio_puts("\n");
    if(pid<=0){
        errno=preverrono;
        return;
    }
    if(kill(-pid,sig)<0)
        Sio_puts("\nSISTSTP kill error\n");
    errno=preverrono;
    return;
}

void child_handler(int sig){
    int preverrono=errno;
    int status;
    pid_t pid;
    while((pid=waitpid(-1,&status,WNOHANG|WUNTRACED))>0){
        int job_id=getjid_by_pid(pid);
        if(WIFSIGNALED(status)){
            Sio_puts("[");
            Sio_putl((long)job_id);
            Sio_puts("]\tTerminated\t");
            Sio_puts(job_list[job_id].cmdline);
            deletejob_by_jobid(job_id);
        }
        else if(WIFSTOPPED(status)){
            Sio_puts("[");
            Sio_putl((long)job_id);
            Sio_puts("]\tStopped\t");
            Sio_puts(job_list[job_id].cmdline);
            job_list[job_id].state='T';
        }
        else if(WIFEXITED(status))
            deletejob_by_pid(pid);
    }
    /*if(errno!=ECHILD)
        Sio_error("wait error");*/
    errno=preverrono;
}
void myjobs(){
    for(int i=0;i<MAXJOB;i++){
        if(job_list[i].job_id==-1)
            continue;
        printf("[%d]\t",job_list[i].job_id);
        switch(job_list[i].state){
            case 'B':
                printf("Running\t");
                break;
            case 'F':
                //printf("Running\t");
                break;
            case 'T':
                printf("Stopped\t");
                break;
            default: 
                printf("somestate\t");
        }
        printf("%s", job_list[i].cmdline);
    }
}
int getjid_by_pid(pid_t pid){
    if(pid<0)
        return -1;
    if(pid==0)
        return 0;
    for(int i=0;i<MAXJOB;i++){
        if(job_list[i].pid==pid)
            return job_list[i].job_id;
    }
    return -1;
}
int getjob_by_pid(pid_t pid){
    if(pid<0)
        return -1;
    for(int i=0;i<MAXJOB;i++)
        if(job_list[i].pid==pid)
            return i;
    return -1;
}
int deletejob_by_pid(pid_t pid){
    if(pid<0)
         return 0;
    for(int i=0;i<MAXJOB;i++){
        if(job_list[i].pid==pid){
            job_list[i].pid=-1;
            job_list[i].job_id=-1;
            job_list[i].state='X';
            job_list[i].cmdline[0]='\0';
            return 1;
        }
    }
    return 0;
}
int deletejob_by_jobid(int job_id){
    if(job_id<0)
        return 0;
    job_list[job_id].pid=-1;
    job_list[job_id].job_id=-1;
    job_list[job_id].state='X';
    job_list[job_id].cmdline[0]='\0';
}
int get_nextjid(){
    int maxjid=0;
    for(int i=0;i<MAXJOB;i++){
        if(job_list[i].job_id>maxjid)
            maxjid=job_list[i].job_id;
    }
    return maxjid+1;
}
int addjob(char* cmdline, pid_t pid, int state){
    if(pid<0)
        return 0;
    int job_id=get_nextjid();
    job_list[job_id].pid=pid;
    job_list[job_id].job_id=job_id;
    job_list[job_id].state=state;
    strcpy(job_list[job_id].cmdline,cmdline);
}
pid_t get_fgpid(){
    for(int i=0;i<MAXJOB;i++){
        if(job_list[i].state=='F')
            return job_list[i].pid;
    }
    return 0;
}
void init_job_list(){
    for(int i=0;i<MAXJOB;i++)
        deletejob_by_jobid(i);
}
void myfg(char* argv[]){
    if(argv[1]==NULL){

    }
}












