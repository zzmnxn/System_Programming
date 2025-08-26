/* $begin shellmain */
#include "csapp.h"
#include "myshell.h"


int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4100-SP-P2> ");                   
	if (fgets(cmdline, MAXLINE, stdin) == NULL) {
        if (feof(stdin)) exit(0);
        continue;
    }
    

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) {
/*****************추가 */
    char *cmdline_copy = strdup(cmdline);  // 원본 보존
    char *cmds[MAXCMDS];
    int num_cmds=split_pipeline(cmdline_copy,cmds);
    // === exit, quit 예외 처리 (부모에서 직접 처리) ===
    if (num_cmds == 1) {
        char *argv[MAXARGS];
        char *cmd_copy = strdup(cmds[0]);  // 파괴 방지용 복사본
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
        pipe(fd);

        pid_t pid = fork();
        if (pid == 0) {  // child
            // 입력 리디렉션
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // 출력 리디렉션 (마지막 명령 제외)
            if (i != num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }

            close(fd[0]);
            close(fd[1]);

            // 명령어 파싱 및 실행
            char *argv[MAXARGS];
            char *cmd_copy = strdup(cmds[i]);  // 여기서도 복사
            parseline(cmd_copy, argv);


            if (!builtin_command(argv)) {
                execvp(argv[0], argv);
                fprintf(stderr, "%s: Command not found\n", argv[0]);
                exit(1);
            }
            free(cmd_copy);
            exit(0); // 내장 명령어 처리 시 종료
        }

        // 부모 프로세스
        close(fd[1]);
        if (in_fd != STDIN_FILENO) close(in_fd);
        in_fd = fd[0];
    }

    for (i = 0; i < num_cmds; i++){
        wait(NULL);
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
        } else if (chdir(argv[1]) != 0) {
            perror("cd");
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
    if(*buf == '\0') break;
    }
    
    argv[argc]=NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */



int split_pipeline(char *cmdline, char **cmds) {
    int count = 0;
    char *token = strtok(cmdline, "|");

    while (token != NULL && count < MAXCMDS) {
        while (*token == ' ') token++; // 왼쪽 공백 제거
        cmds[count++] = strdup(token); //문자열 복사해서 원본 보장장
        token = strtok(NULL, "|");
    }
    cmds[count] = NULL;
    return count;
}