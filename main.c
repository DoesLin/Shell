#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "shell_fct.h"

int main(int argc, char** argv)
{
  //Initialize
  DEBUG("Initializing my shell");
  int ret = MYSHELL_CMD_OK;
  char* readlineptr = NULL;
  struct passwd* infos = NULL;
  char str[1024] = {'\0'};
  char hostname[256] = {'\0'};
  char workingdirectory[256] = {'\0'};
  int sigBoot = 0;

  //..........
  while(ret != MYSHELL_FCT_EXIT) {
    //Get your session info
    infos=getpwuid(getuid());
    gethostname(hostname, 256);
    getcwd(workingdirectory, 256);

    if(sigBoot == 0) {
      DEBUG("My shell is ready");
      sigBoot = 1;
    }

    //Print it to the console
    sprintf(str, "\n{myshell}%s@%s:%s$ ", infos->pw_name, hostname, workingdirectory);
    fflush(stdin);
    readlineptr = readline(str);

    /* If the line has any text in it, save it on the history. */
    // \author Y. LIN
    if(readlineptr!=NULL && strcmp(readlineptr, "")) {
      add_history(readlineptr);

      //Your code goes here.......
      cmd my_cmd;
      //Parse the comand
      if(!parseMembers(readlineptr, &my_cmd)) {
        if(ISDEBUG){
          printCmd(&my_cmd);
        }
        //Execute the comand
        exec_command(&my_cmd);
      }
      //Clean the house
      freeCmd(&my_cmd);
    } else {
      printf("Command is null.\n");
    }
    free(readlineptr);
    //..........
  }
  //..........
  return 0;
}
