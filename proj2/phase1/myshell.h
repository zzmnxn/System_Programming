// myshell.h
#ifndef MYSHELL_H
#define MYSHELL_H

#define MAXARGS   128
#define MAXLINE   1024  // csapp.h���� ���ǵǾ� �ִ� ��� ���� ����

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

#endif // MYSHELL_H
