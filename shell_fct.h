#ifndef MYSHELL_SHELL_FCT_H
#define MYSHELL_SHELL_FCT_H

#include "cmd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

//Terminate shell
#define MYSHELL_FCT_EXIT 1

//Execute a command
int exec_command(cmd *c);

#endif
