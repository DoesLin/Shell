#include "shell_fct.h"

/** \brief fatalError
 * A function which processes error message and exit
 * when the program has detected a fatal error
 * \author Y. LIN
 * \param const char *msg: An error message
 * \return None
 *
 */
static void fatalError(const char *msg) {
  perror(msg);
  exit(errno);
}

/** \brief fatalError
 * A function which processes error message and exit
 * when the program has detected a reboot error
 * \author Y. LIN
 * \param const char *msg: An error message
 * \return None
 *
 */
static void rebootError(const char *msg) {
  printf("%s\n", msg);
  exit(0);
}

int *pidChd = NULL;
int cmdNbr = 0;

/*Realizes shell builtin commands*/
static int builtin_command(cmd *cmd){
  char* username;
  char workingdirectory[256] = "/home/";
  unsigned int cpt;

  // Upgrates for the exit/cd/pwd bugs
  // \author Y. LIN
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    //Pwd is the highest
    if(!strcmp(cmd->cmdMembersArgs[cpt][0], "pwd")) {
      printf("%s\n", getcwd(workingdirectory, sizeof(workingdirectory)));
      return 1;
    }
  }

  // Upgrates for the exit/cd/pwd bugs
  // \author Y. LIN
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    if(!strcmp(cmd->cmdMembersArgs[cpt][0], "exit")) {
      if(cmd->nbCmdMembers==1) {
        exit(0);
      } else {
        printf("Command is wrong format.\n");
        return 1;
      }
    } else if(!strcmp(cmd->cmdMembersArgs[cpt][0], "cd")) {
      if(cmd->nbCmdMembers==1) {
        /*To a user's home directory*/
        if( cmd->nbMembersArgs[0]==1 || !strcmp(cmd->cmdMembersArgs[0][1], "~")) {
          username = getenv("USER");
          DEBUG("Getusername: %s\n", username);
	        strcat(workingdirectory, username);
	        DEBUG("Workingdirectory: %s\n", workingdirectory);
	        chdir(workingdirectory);
        }

        else if(chdir(cmd->cmdMembersArgs[cpt][1]) != 0) {
	        printf("-myshell: cd: %s:%s\n", cmd->cmdMembersArgs[0][1], strerror(errno));
        }

        return 1;
      } else {
        printf("Command has wrong format\n");
        return 1;
      }
    }
  }

  return 0;
}

/*Signal handler*/
/** \brief my_alarm_handler
 * A signal handler which handles the following steps
 * after the alarm times up
 * \author Y. LIN
 * \param const char *msg: An error message
 * \return None
 *
 */
static void my_alarm_handler(int signo) {
  printf("Command executed time out.\n");
  kill(pidChd[cmdNbr], SIGKILL);
}

int exec_command(cmd* cmd) {
  /*The number of the cmd in execution*/
  int cmdNo;

  /*Number of pipes*/
  int pipe_num = cmd->nbCmdMembers - 1;

  /*Used to redirect input and output*/
  int fd_in, fd_out, fd_err;

  // Uses for cycles
  unsigned int cpt;

  // Upgrates whether command's member is incomplete
  // \author Y. LIN
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    if(!strcmp("", cmd->cmdMembers[cpt])) {
      printf("Command's member is incomplete.\n");
      return 0;
    } else {
      DEBUG("cmdMembers[cpt]: %s", cmd->cmdMembers[cpt]);
    }
  }

  /*It's a buildin command*/
  if(builtin_command(cmd)) {
    return 0;
  }

  /*Create 'cmd->nbCmdMembers - 1' pipes*/
  int **pipe_fd = calloc(pipe_num, sizeof(int *));
  for(int i = 0;i < pipe_num;i++) {
    pipe_fd[i] = calloc(2, sizeof(int));
    pipe(pipe_fd[i]);
  }

  //Create 'cmd->nbCmdMembers' pids
  int *statusChd = (int *)calloc(cmd->nbCmdMembers, sizeof(int));
  pidChd = (int *)calloc(cmd->nbCmdMembers, sizeof(int));
  cmdNbr = 0;

  // Shell will finish If it's not responded more than 5 seconds
  // \author Y. LIN
  signal(SIGALRM, my_alarm_handler);
  alarm(5);
  DEBUG("Alarm set.");

  /*Create child process for each cmd*/
  for(cmdNo = 0; cmdNo < cmd->nbCmdMembers; cmdNo++) {
    cmdNbr = cmdNo;
    if((pidChd[cmdNo] = fork()) < 0) {
      fatalError("Fork fail!");
    }
    if(pidChd[cmdNo] == 0) {
      break;//Child break and 'cmdNo' is the serial number
    }
  }

  if(pidChd[cmdNbr] == 0) {
    DEBUG("Child: cmdNo = %d", cmdNo);

    /*Redirect input*/
    if(cmd->redirection[cmdNo][STDIN_FILENO] != NULL) {
      if((fd_in = open(cmd->redirection[cmdNo][STDIN_FILENO], O_RDONLY)) < 0) {
        rebootError("Open fail!");
      }
      dup2(fd_in, 0);
      close(fd_in);
    }

    /*Redirect output*/
    if(cmd->redirection[cmdNo][STDOUT_FILENO] != NULL) {

      /*The file is opened in append mode*/
      if(!strcmp((char*)cmd->redirectionType[cmdNo][0], "APPEND")) {
	       if((fd_out = open(cmd->redirection[cmdNo][STDOUT_FILENO], O_RDWR | O_CREAT | O_APPEND, 0666)) < 0) {
	          rebootError("Open fail!");
	       }
      } else {

	       /*The file will be truncated to length 0*/
	        if((fd_out = open(cmd->redirection[cmdNo][STDOUT_FILENO], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
            rebootError("Open fail!");
	        }
      }
      dup2(fd_out, 1);
      close(fd_out);
    }

    /*Redirect error output*/
    if(cmd->redirection[cmdNo][STDERR_FILENO] != NULL) {

      /*The file is opened in append mode*/
      if(!strcmp((char*)cmd->redirectionType[cmdNo][1], "APPEND")) {
	       if((fd_err = open(cmd->redirection[cmdNo][STDERR_FILENO], O_RDWR | O_CREAT | O_APPEND, 0666)) < 0) {
	          rebootError("Open fail!");
	       }
      } else {

	       /*The file will be truncated to length 0*/
	        if((fd_err = open(cmd->redirection[cmdNo][STDERR_FILENO], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
            rebootError("Open fail!");
	        }
      }
      dup2(fd_err, 2);
      close(fd_err);
    }

    /*If there are pipes in the process*/
    if(pipe_num) {

      /*For the first cmd*/
      if(cmdNo == 0) {
        close(pipe_fd[cmdNo][0]);
	      dup2(pipe_fd[cmdNo][1], 1);
	      close(pipe_fd[cmdNo][1]);

	      /*Close other pipes which it doesn't need*/
	      for(int i = 1;i < pipe_num;i++) {
	         close(pipe_fd[i][0]);
	         close(pipe_fd[i][1]);
         }
       }

      /*For the last cmd*/
      else if(cmdNo == pipe_num) {
        close(pipe_fd[cmdNo - 1][1]);
        dup2(pipe_fd[cmdNo - 1][0], 0);
        close(pipe_fd[cmdNo - 1][0]);

	      /*Close other pipes which it doesn't need*/
	      for(int i = 0;i < pipe_num - 1;i++) {
	         close(pipe_fd[i][0]);
	         close(pipe_fd[i][1]);
	      }
      }

      /*For other cmds in the middle*/
      else {
        dup2(pipe_fd[cmdNo - 1][0], 0);
        close(pipe_fd[cmdNo - 1][0]);

        dup2(pipe_fd[cmdNo][1], 1);
	      close(pipe_fd[cmdNo][1]);

	      /*Close other pipes which it doesn't need*/
	      for(int i = 0;i < pipe_num;i++) {
	         if((i != cmdNo - 1) || (i != cmdNo)) {
	            close(pipe_fd[i][0]);
	            close(pipe_fd[i][1]);
	         }
        }
      }
    }
    // Already handled the situation of unrecognized file name
    execvp(cmd->cmdMembersArgs[cmdNo][0], cmd->cmdMembersArgs[cmdNo]);
  }

  /*Parent loves them*/
  for(int i = 0;i < pipe_num;i++) {
    close(pipe_fd[i][0]);
    close(pipe_fd[i][1]);
  }

  // \author Y. LIN
  for(int i = 0;i < cmd->nbCmdMembers;i++) {
    waitpid(pidChd[i], &statusChd[i], 0);
    if(WIFEXITED(statusChd[i])) {
      if(errno!=0) {
        // A error happens when it's executed
        printf("An error happens when executes this command\n");
        printf("The number of error's type is %d\n", errno);
        exit(0);
      } else {
        DEBUG("Child process successfully completed");
        alarm(0);
      }
    } else {
      DEBUG("Child process unsuccessfully completed");
    }
  }
  DEBUG("Father: End all the waiting.");

  /*free the array*/
  for(int i = 0;i < pipe_num;i++) {
    free(pipe_fd[i]);
  }
  free(pipe_fd);
  free(pidChd);
  free(statusChd);

  return 0;
}
