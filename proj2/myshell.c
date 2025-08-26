#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>    
#include <sys/types.h>
#include <sys/wait.h>

#include "csapp.h"
#include "myshell.h"


Job job_list[MAXJOBS];    
int next_job_id = 1;        
volatile pid_t fg_pid = 0; // foreground pid �����
volatile sig_atomic_t PID = 0; //SIGCHILD�� eval �� ����ȭ��


int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    
    init_sighandlers();

    while (1) {
	/* Read */
	Sio_puts("CSE4100-SP-P2> ");                   
	if (fgets(cmdline, MAXLINE, stdin) == NULL) {
        if (feof(stdin)) exit(0);
        continue;
    }
    // & �پ� �ִ� ��쵵 ó��: sleep& -> sleep &
    for (int i = 0; cmdline[i]; i++) {
        if (cmdline[i] == '&' && (i == 0 || cmdline[i-1] != ' ')) {
            cmdline[i] = '\0';
            strcat(cmdline, " &\n");
            break;
        }
    }

    
	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) {
/*****************�߰� */
    int only_whitespace = 1;
    for (int i = 0; cmdline[i]; i++) {
        if (cmdline[i] != ' ' && cmdline[i] != '\n') {
            only_whitespace = 0;
            break;
        }
    }
    if(only_whitespace) return;

    sigset_t mask_all, mask_one, prev;
    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD); // SIGCHLD�� block��

    int bg=0;  
    char *cmdline_copy = strdup(cmdline);  // ���� ����
    char *cmds[MAXCMDS];
    int num_cmds=split_pipeline(cmdline_copy,cmds);

    char *tmp = strdup(cmds[num_cmds - 1]);
    bg = parseline(tmp, NULL);  // argv ���� bg ���θ� �Ǵ�
    free(tmp);

    // === exit, quit ���� ó�� (�θ𿡼� ���� ó��) ===
    if (num_cmds == 1) {
        char *argv[MAXARGS];
        char *cmd_copy = strdup(cmds[0]);  // �ı� ������ ���纻
        parseline(cmd_copy, argv);
        //printf("[DEBUG] argv[0]: %s\n", argv[0]); 
        if (builtin_command(argv)) {
            free(cmd_copy);
            free(cmdline_copy);
            return;
        }
        free(cmd_copy);
    }
    int i;
    int in_fd = STDIN_FILENO;
    int fd[2];
    
    for (i = 0; i < num_cmds; i++) {
        if (pipe(fd) < 0) {
            unix_error("pipe error");
        } 

        Sigprocmask(SIG_BLOCK, &mask_all, &prev);

        pid_t pid = Fork();
        if (pid == 0) {  // child
            // �Է� ���𷺼�
            Signal(SIGCHLD, SIG_DFL);
            Signal(SIGINT, SIG_DFL);
            Signal(SIGTSTP,SIG_DFL);
            
            Sigprocmask(SIG_SETMASK, &prev, NULL);
            Setpgid(0,0);
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // ��� ���𷺼� (������ ��� ����)
            if (i != num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }

            close(fd[0]);
            close(fd[1]);

            // ��ɾ� �Ľ� �� ����
            char *argv[MAXARGS];
            char *cmd_copy = strdup(cmds[i]);  // ���⼭�� ����
            parseline(cmd_copy, argv);

            if (!builtin_command(argv)) {
                execvp(argv[0], argv);
                fprintf(stderr, "%s: Command not found\n", argv[0]);
                fflush(stderr);
                exit(1);
            }
            free(cmd_copy);
            exit(0); // ���� ��ɾ� ó�� �� ����
        }

        // �θ� ���μ���
        Setpgid(pid, pid);
        if (i == num_cmds - 1) {
            if(!bg) fg_pid=pid; //foreground job �̸� ����
            addjob(pid, bg ? Background : Foreground, cmdline);
            
        }

        Sigprocmask(SIG_SETMASK, &prev, NULL); 

        close(fd[1]);
        if (in_fd != STDIN_FILENO) close(in_fd);
        in_fd = fd[0];
    }

     if (!bg) {
        int status;
        PID = 0;
        while (!PID) {
            if (foreground_check())
                Sigsuspend(&prev);
            else break;
        }
        fg_pid = 0;
    }
    for (i = 0; i < num_cmds; i++) {
        free(cmds[i]);
    }
    free(cmdline_copy);
    

}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")||!strcmp(argv[0],"exit")) //quit command */
	exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    if (!strcmp(argv[0], "cd")) {
        if (argv[1] == NULL) {
            printf("cd: missing argument\n");
            fflush(stderr);
        } else if (chdir(argv[1]) != 0) {
            perror("cd");
        }
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) {
        listjobs();
        return 1;
    }
    if (!strcmp(argv[0], "fg")) {
        int jid = atoi(argv[1]);
        Job *job = getjob_by_id(jid);
        if (!job) {
            printf("fg: No such job\n");
            fflush(stderr);
            return 1;
        }
        job->state = Foreground;
        fg_pid=job->pid;
        Kill(-job->pid, SIGCONT);
        int status;
        waitpid(job->pid, &status, 0);
        deletejob(job->pid);
        
        return 1;
    }
    if (!strcmp(argv[0], "bg")) {
        int jid = atoi(argv[1]);
        Job *job = getjob_by_id(jid);
        if (job && job->state == Stopped) {
            job->state = Background;
            Kill(-job->pid, SIGCONT);
        }
        return 1;
    }
    if (!strcmp(argv[0], "kill")) {
        int jid = atoi(argv[1]);
        Job *job = getjob_by_id(jid);
        if (job) Kill(-job->pid, SIGKILL);
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
    

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')){ /* Ignore leading spaces */
	    buf++;
    }  
    if (argv == NULL) {  // ? argv�� NULL�̸� bg ���θ� �Ǵ�
        if (strstr(buf, "&")) return 1;
        return 0;
    }

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    if(*buf == '\0') break;
    }
    
    argv[argc]=NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 0;

    int bg=0;
    if (*argv[argc - 1] == '&') {
        bg = 1;
        argv[--argc] = NULL;
    }
    return bg;

    /* Should the job run in the background? */
    // parseline �� �κп� ������ ���� ����

}

/* $end parseline */



int split_pipeline(char *cmdline, char **cmds) {
    int count = 0;
    char *token = strtok(cmdline, "|");

    while (token != NULL && count < MAXCMDS) {
        while (*token == ' ') token++; // ���� ���� ����
        cmds[count++] = strdup(token); //���ڿ� �����ؼ� ���� ������
        token = strtok(NULL, "|");
    }
    cmds[count] = NULL;
    return count;
}

/***********************phase3 job ���� �Լ��� */

void initjobs() {
    for (int i = 0; i < MAXJOBS; i++) {
        job_list[i].state = Invalid;
        job_list[i].last_idx = 0;
    }
}

int get_empty_index() {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state == Invalid)
            return i;
    }
    return -1;
}

void addjob(pid_t pid, State state, const char *cmdline) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state == Invalid) {
            job_list[i].job_id = next_job_id++;
            job_list[i].pid = pid;
            job_list[i].state = state;
            strncpy(job_list[i].cmdline, cmdline, MAXLINE);
            job_list[i].cmdline[strcspn(job_list[i].cmdline, "\n")] = '\0';
            job_list[i].last_idx = (i == MAXJOBS - 1);
            return;
        }
    }
    Sio_puts("Job list full\n");

}

void deletejob(pid_t pid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == pid) {
            job_list[i].state = Invalid;
            return;
        }
    }
}

Job *getjob(pid_t pid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == pid)
            return &job_list[i];
    }
    return NULL;
}

Job *getjob_by_id(int jid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].job_id == jid)
            return &job_list[i];
    }
    return NULL;
}

void listjobs() {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state != Invalid) {
            char *state_str = (job_list[i].state == Background) ? "Running" :
                              (job_list[i].state == Foreground) ? "Foreground" :
                              (job_list[i].state == Stopped) ? "Stopped" : "Invalid";
            printf("[%d] (%d) %-10s %s\n", job_list[i].job_id, job_list[i].pid, state_str, job_list[i].cmdline);

        }
    }
}

int foreground_check(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state == Foreground)
            return 1;
    }
    return 0;
}


/*******************�ñ׳� �ڵ鷯 */
void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED|WCONTINUED)) > 0) {
        //if (pid == fg_pid) continue; // foreground�� eval()���� ó��
        Job *job = getjob(pid);

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            deletejob(pid);
            if(pid==fg_pid) PID=pid;
        } else if (WIFSTOPPED(status)) {
            if(job) job->state = Stopped;
        }
        else if (WIFCONTINUED(status)) {
            // ? bg 1�� �ٽ� ����� job�� ����� ���;� ��
            if (job && job->state == Stopped)
                job->state = Background;
        }
    
    }
    errno = olderrno;
}

void sigint_handler(int sig) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state != Invalid && job_list[i].state == Foreground) {
            Kill(-job_list[i].pid, SIGINT);  // �Ǵ� SIGTSTP
            break;
        }
    }
    
}

void sigtstp_handler(int sig) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_list[i].state != Invalid &&
            job_list[i].state == Foreground) {
            Kill(-job_list[i].pid, SIGTSTP);
            job_list[i].state=Stopped;
            break;
        }
    }
}

void init_sighandlers(){
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);
    initjobs();
}