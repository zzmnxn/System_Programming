// myshell.h
#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define MAXARGS 128
#define MAXLINE 1024
#define MAXCMDS 16

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int split_pipeline(char *cmdline, char **cmds);

#endif // MYSHELL_H
