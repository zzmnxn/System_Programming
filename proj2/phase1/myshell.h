// myshell.h
#ifndef MYSHELL_H
#define MYSHELL_H

#define MAXARGS   128
#define MAXLINE   1024  // csapp.h에서 정의되어 있는 경우 생략 가능

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

#endif // MYSHELL_H
