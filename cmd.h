#ifndef MYSHELL_CMD_H
#define MYSHELL_CMD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//Command is well-formed
#define MYSHELL_CMD_OK 0
#define APPEND 1
#define OVERRIDE 2

//To print the command
#define __DEBUG__

#ifdef __DEBUG__
#define ISDEBUG 1
// #define DEBUG(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)
#define DEBUG(format,...) //
#else
#define ISDEBUG 0
#define DEBUG(format,...)
#endif

typedef struct {
    //the command originally inputed by the user
    char *initCmd;

    //number of members
    unsigned int nbCmdMembers;

    //each position holds a command member
    char **cmdMembers;

    //cmd_members_args[i][j] holds the jth argument of the ith member
    char ***cmdMembersArgs;

    //number of arguments per member
    unsigned int *nbMembersArgs;

    //the path to the redirection file
    char ***redirection;

    //the redirection type (append vs. override)
    char ***redirectionType;
} cmd;

//Prints the command
void printCmd(cmd *cmd);
//Frees memory associated to a cmd
void freeCmd(cmd *cmd);
//Frees memory associated to a error cmd
void freeErrorCmd(cmd *cmd);
//Initializes the initial_cmd, membres_cmd et nb_membres fields
int parseMembers(char *s, cmd *c);

#endif
