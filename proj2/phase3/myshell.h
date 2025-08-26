// myshell.h
#ifndef MYSHELL_H
#define MYSHELL_H

#include <signal.h>

#define MAXARGS 128
//#define MAXLINE 1024
#define MAXCMDS 16
#define MAXJOBS 50



typedef enum { Invalid, Foreground, Background, Stopped } State;

typedef struct {
    pid_t pid;
    char cmdline[MAXLINE];
    int job_id;
    State state;
    int last_idx;
} Job;

extern Job job_list[MAXJOBS];
extern int next_job_id;

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int split_pipeline(char *cmdline, char **cmds);
/*********phase3 */

void addjob(pid_t pid, State state, const char *cmdline);
void deletejob(pid_t pid);
Job *getjob(pid_t pid);
Job *getjob_by_id(int jid);
void listjobs();
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void init_sighandlers();
int foreground_check();  
char *insert_special_spaces(const char *cmdline);

#endif // MYSHELL_H
