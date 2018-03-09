#include "cmd.h"

/** \brief formatError
 * A function which processes the data of redirection and its type
 * when the input format has been detected an error
 * \author Y. LIN
 * \param char ***redirection: A pointer which points to the redirection
 * \param char ***redirectionType: A pointer which points to the redirectionType
 * \return None
 *
 */
static void formatError(char ***redirection, char ***redirectionType) {
  free((*redirection)[STDIN_FILENO]);
  free((*redirection)[STDOUT_FILENO]);
  free((*redirection)[STDERR_FILENO]);
  free(*redirection);
  *redirection=NULL;

  free((*redirectionType)[0]);
  free((*redirectionType)[1]);
  free(*redirectionType);
  *redirectionType=NULL;
}

/** \brief detectDangerChar
 * A function which detects whether the current character is dangerous or not
 * \author Y. LIN
 * \param const char curChar: Current character
 * \return 0: when it's safe; 1: when it's dangerous
 *
 */
static int detectDangerChar(const char curChar) {
    if((curChar==' ') ||
       (curChar>=(char)46 && curChar<=(char)57) ||
       (curChar>=(char)65 && curChar<=(char)90) ||
       (curChar>=(char)97 && curChar<=(char)122)) {
        // Safe char
        // Space or number or alphphabet
        return 0;
    } else {
        // Dangerous char
        return 1;
    }
}

/** \brief detectSpecialStd
 * A function which detects whether the current character is
 * a special std redirection input
 * \author Y. LIN
 * \param const char *curInput: Current input
 * \return 0: when it is not special; 1: when it's special; -1: when it is "2>&1"
 *
 */
static int detectSpecialStd(const char *curInput) {
  if((*curInput)=='0') {
        // 0<
        if(*(curInput+1)=='<') {
            return 1;
        }
    } else if((*curInput)=='1') {
        // 1>
        if(*(curInput+1)=='>') {
            return 1;
        }
    } else if((*curInput)=='2') {
        // 2>*
        if(*(curInput+1)=='>') {
            // 2>&1
            if(!strncmp(curInput, "2>&1", 4)) {
                return -1;
            }
            return 1;
        }
    } else if((*curInput)=='&') {
        // &>
        if(*(curInput+1)=='>') {
            return 1;
        }
    }
    return 0;
}

/** \brief cmdInit
 * Initializes the pointer of the command
 * \author Y. LIN
 * \param cmd *cmd: A pointer which points to the command
 * \return None
 *
 */
static void cmdInit(cmd *cmd) {
  cmd->initCmd=strdup("");
  cmd->nbCmdMembers=0;
  cmd->cmdMembers=NULL;
  cmd->cmdMembersArgs=NULL;
  cmd->redirection=NULL;
  cmd->redirectionType=NULL;
}

/** \brief deleteBeginningBlank
 * A function which deletes the spaces in the beginning of current input
 * \author Y. LIN
 * \param const char *curInput: Current input
 * \return None
 *
 */
static void deleteBeginningBlank(char **curInput) {
  while((**curInput)==' ') {(*curInput)++;}
}

/** \brief deleteBackBlank
 * A function which deletes the spaces in the end of current input
 * \author Y. LIN
 * \param const char *curInput: Current input
 * \return None
 *
 */
static void deleteBackBlank(char **curInput, size_t *memLen) {
  (*curInput)--;
  while((**curInput)==' ') {
    (*memLen)--;
    (*curInput)--;
  }
  (*curInput)++;
}

/** \brief getMemberArg
 * A function which gets the member's arguments and updates the number of arguments
 * \author Y. LIN
 * \param char ***cmdMembersArgs: A pointer which points to the member's arguments
 * \param const char *cmdMembers: A pointer which points to the current member of command
 * \param unsigned int *nbMembersArgs: A pointer which points to the number of member's arguments
 * \return None
 *
 */
static void getMemberArg(char ***cmdMembersArgs, const char *cmdMembers, unsigned int *nbMembersArgs) {
  *cmdMembersArgs=(char **)malloc(sizeof(char *));
  **cmdMembersArgs=NULL;
  *nbMembersArgs=0;

  while((*cmdMembers)!='<'&&(*cmdMembers)!='>'&&(*cmdMembers)!='\0') {
      size_t argLen=0;
      while((*cmdMembers)!=' '&&(*cmdMembers)!='<'&&(*cmdMembers)!='>'&&(*cmdMembers)!='\0') {
          argLen++;
          cmdMembers++;
      }
      *cmdMembersArgs=(char **)realloc(*cmdMembersArgs, (size_t)(*nbMembersArgs+2)*sizeof(char *));
      (*cmdMembersArgs)[*nbMembersArgs]=strndup(cmdMembers-argLen, argLen);
      (*cmdMembersArgs)[*nbMembersArgs+1]=NULL;
      (*nbMembersArgs)++;
      while(*cmdMembers==' ') {cmdMembers++;}
      if(detectSpecialStd(cmdMembers)) {
          // Beyond the special number
          while((*cmdMembers)==' ') {cmdMembers++;}
          cmdMembers++;
      }
  }
}

/** \brief getRedirection
 * A function which gets the member's redirection and updates the type of redirection
 * \author Y. LIN
 * \param char ***redirection: A pointer which points to the redirection
 * \param const char *cmdMembers: A pointer which points to the current member of command
 * \param char ***redirectionType: A pointer which points to the redirectionType
 * \return None
 *
 */
static void getRedirection(char ***redirection, const char *cmdMembers, char ***redirectionType) {
  //0: STDIN  1: STDOUT   2: STDERR
  *redirection=(char **)malloc(3*sizeof(char *));
  (*redirection)[0]=NULL;
  (*redirection)[1]=NULL;
  (*redirection)[2]=NULL;
  //0: STDOUT  1: STDERR
  *redirectionType=(char **)malloc(2*sizeof(char *));
  (*redirectionType)[0]=NULL;
  (*redirectionType)[1]=NULL;
  int dirType=0;

  while(*cmdMembers!='\0' && dirType!=-1) {
    int spcStd = 0;
    while((*cmdMembers)!='<' && (*cmdMembers)!='>' && (*cmdMembers)!='\0') {
        spcStd=detectSpecialStd(cmdMembers);
        if(spcStd) {
            break;
        } else {
            cmdMembers++;
        }
    }
    // If it hasn't redirection
    if(*cmdMembers=='\0') {
      free(*redirectionType);
      *redirectionType=NULL;
      break;
    }

    dirType=-1;
    size_t dirLen=0;
    int dangerChar=0;

    // STDIN: <, 0<
    if(!strncmp(cmdMembers, "<", 1) || !strncmp(cmdMembers, "0<", 2)) {
        if(!strncmp(cmdMembers, "<", 1)) {
          dangerChar=detectDangerChar(*(cmdMembers+1));
        } else {
          dangerChar=detectDangerChar(*(cmdMembers+2));
        }
        if(dangerChar) {
          while((*cmdMembers)!=' '&&(*cmdMembers)!='\0') {cmdMembers++;}
          dirType=-1;
        } else {
          dirType=STDIN_FILENO;
        }
    // STDOUT: >, >>, 1>
    } else if(!strncmp(cmdMembers, ">", 1) ||
              !strncmp(cmdMembers, ">>", 2) ||
              !strncmp(cmdMembers, "1>", 2)) {

        if(!strncmp(cmdMembers, ">>", 2) ||
           !strncmp(cmdMembers, "1>", 2)) {
          dangerChar=detectDangerChar(*(cmdMembers+2));
        } else {
          dangerChar=detectDangerChar(*(cmdMembers+1));
        }
        if(dangerChar) {
          while((*cmdMembers)!=' '&&(*cmdMembers)!='\0') {cmdMembers++;}
          dirType=-1;
        } else {
          dirType=STDOUT_FILENO;
          free((*redirectionType)[0]);
          if(!strncmp(cmdMembers, ">>", 2)) {
            (*redirectionType)[0]=strdup("APPEND");
          } else {
            (*redirectionType)[0]=strdup("OVERRIDE");
          }
        }
    // STDERR: 2>, 2>>, &>, 2>&1
    } else if(!strncmp(cmdMembers, "2>>", 3) ||
              !strncmp(cmdMembers, "2>", 2) ||
              !strncmp(cmdMembers, ">&", 2) ||
              !strncmp(cmdMembers, "2>&1", 4)) {
        // 2>&1 and &> sends the output of the file descriptor 2, stderr
        // to the same location as the file descriptor 1, stdout.
        if(!strncmp(cmdMembers, "2>&1", 4)) {
          dangerChar=!(*(cmdMembers+4)==' ');
        } else if(!strncmp(cmdMembers, "2>>", 3)) {
          dangerChar=detectDangerChar(*(cmdMembers+3));
        } else {
          dangerChar=detectDangerChar(*(cmdMembers+2));
        }
        if(dangerChar) {
          while((*cmdMembers)!=' '&&(*cmdMembers)!='\0') {cmdMembers++;}
          dirType=-1;
        } else {
          dirType=STDERR_FILENO;
          free((*redirectionType)[1]);
          if(!strncmp(cmdMembers, "2>>", 3)) {
            (*redirectionType)[1]=strdup("APPEND");
          } else {
            (*redirectionType)[1]=strdup("OVERRIDE");
            if(!strncmp(cmdMembers, "2>&1", 4) || !strncmp(cmdMembers, "&>", 2)) {
              free((*redirectionType)[0]);
              (*redirectionType)[0]=strdup("OVERRIDE");
              dirType=3;
            }
          }
        }
    // Unrecognized redirection format
    } else {
      while((*cmdMembers)!=' '&&(*cmdMembers)!='\0') {cmdMembers++;}
      dirType=-1;
    }

    // Arrive the beginning of the redir's name
    if(1==spcStd) {
      // Beyond the special number
      while((*cmdMembers)==' ') {cmdMembers++;}
      if(strlen(cmdMembers)>1) {
        cmdMembers+=1;
      } else {
        dirType=-1;
      }
    } else if(-1==spcStd) {
      // Beyond the special number -- "2>&1"
      while((*cmdMembers)==' ') {cmdMembers++;}
      if(strlen(cmdMembers)>4) {
        cmdMembers+=4;
      } else {
        dirType=-1;
      }
    }
    while((*cmdMembers)!='\0' &&
          ((*cmdMembers)=='>'||(*cmdMembers)=='<'||(*cmdMembers)==' ')) {cmdMembers++;}

    // Get the length of redir's name
    while((*cmdMembers)!='\0'&&(*cmdMembers)!=' ') {
      cmdMembers++;
      dirLen++;
    }

    switch(dirType) {
    // Wrong redirection
    case -1:
      formatError(redirection, redirectionType);
      printf("Unrecognized redirection format\n");
      break;
    case STDIN_FILENO:
      free((*redirection)[0]);
      (*redirection)[0]=strndup(cmdMembers-dirLen, dirLen); break;
    case STDOUT_FILENO:
      free((*redirection)[1]);
      (*redirection)[1]=strndup(cmdMembers-dirLen, dirLen); break;
    case STDERR_FILENO:
      free((*redirection)[2]);
      (*redirection)[2]=strndup(cmdMembers-dirLen, dirLen); break;
    // Stderr & stdout redirection
    case 3:
      free((*redirection)[1]);
      free((*redirection)[2]);
      (*redirection)[1]=strndup(cmdMembers-dirLen, dirLen);
      (*redirection)[2]=strndup(cmdMembers-dirLen, dirLen);
      break;
    default: ;
    }
  }

  // If the redirection don't have type
  if((*redirectionType)!=NULL && (*redirectionType)[0]==NULL && (*redirectionType)[1]==NULL) {
    free(*redirectionType);
    *redirectionType=NULL;
  }

}

/** \brief printCmd
 * A function which prints informations associated to a command
 * \author Y. LIN
 * \param cmd *cmd: A pointer which points to the command
 * \return None
 *
 */
void printCmd(cmd *cmd) {
  unsigned int cpt;
  printf("*****TESTCMD*****\n");
  // Prints commands
  printf("init_cmd: %s\n", (cmd->initCmd)==NULL? "NULL":cmd->initCmd);
  // Prints number of commands's members
  printf("nb_cmd_members: %d\n", cmd->nbCmdMembers);
  // Prints commands
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
      printf("cmd_members [%d]: %s\n", cpt, (cmd->cmdMembers[cpt])==NULL? "NULL":cmd->cmdMembers[cpt]);
  }
  // Prints commands's members
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    unsigned int cptArg;
    for(cptArg=0; cptArg<cmd->nbMembersArgs[cpt]+1; cptArg++){
      printf("cmd_members_args [%d][%d]: %s\n", cpt, cptArg, (cmd->cmdMembersArgs[cpt][cptArg])==NULL?
            "NULL":cmd->cmdMembersArgs[cpt][cptArg]);
    }
  }
  // Prints commands's members' arguments
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    printf("nb_members_args [%d]: %d\n", cpt, cmd->nbMembersArgs[cpt]);
  }
  // Prints commands's members' redirections
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    printf("redirection[%d][STDIN]: %s\n", cpt,
         (cmd->redirection[cpt][STDIN_FILENO])==NULL? "NULL":cmd->redirection[cpt][STDIN_FILENO]);
    printf("redirection[%d][STDOUT]: %s\n", cpt,
         (cmd->redirection[cpt][STDOUT_FILENO])==NULL? "NULL":cmd->redirection[cpt][STDOUT_FILENO]);
    printf("redirection[%d][STDERR]: %s\n", cpt,
         (cmd->redirection[cpt][STDERR_FILENO])==NULL? "NULL":cmd->redirection[cpt][STDERR_FILENO]);
  }
  // Prints commands's members' redirections' types
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    if(cmd->redirectionType[cpt]!=NULL) {
      printf("redirection_type[%d][STDOUT]: %s\n", cpt,
             (cmd->redirectionType[cpt][0])==NULL? "NULL":cmd->redirectionType[cpt][0]);
      printf("redirection_type[%d][STDERR]: %s\n", cpt,
             (cmd->redirectionType[cpt][1])==NULL? "NULL":cmd->redirectionType[cpt][1]);
    } else {
      printf("redirection_type[%d]: NULL\n", cpt);
    }
  }
  printf("*****************\n");
}

/** \brief parseMembers
 * A function which parses the command's members of current input
 * Including initializing the initial_cmd, membres_cmd et nb_membres fields
 * \author Y. LIN
 * \param char *inputString: A pointer which points to the current input
 * \param cmd *cmd: A pointer which points to the command
 * \return 0: when the current input format is correct; 1: when the current input format is not correct
 *
 */
int parseMembers(char *inputString, cmd *cmd){
    char *curIpt=inputString;
    unsigned int cpt;
    cmdInit(cmd);

    free(cmd->initCmd);
    cmd->initCmd=strdup(inputString);
    cmd->nbCmdMembers=1;

    //Get number of commands
    while(*curIpt != '\0') {
        if(*curIpt=='|') {
            cmd->nbCmdMembers++;
        }
            curIpt++;
    }

    curIpt=inputString;
    cmd->cmdMembers=(char **)malloc(sizeof(char *)*(size_t)cmd->nbCmdMembers);
    for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
        size_t memLen=0;

        //delete stared blank
        deleteBeginningBlank(&curIpt);
        while(*curIpt!='|' && *curIpt!='\0') {
            memLen++;
            curIpt++;
        }

        //delete back blank
        deleteBackBlank(&curIpt, &memLen);
        curIpt-=memLen;

        //Get cmdMembers
        cmd->cmdMembers[cpt]=strndup(curIpt, memLen);

        //Get redirection
        if(cmd->redirection==NULL) {
            cmd->redirection=(char ***)malloc(sizeof(char **));
            cmd->redirectionType=(char ***)malloc(sizeof(char **));
        } else {
            cmd->redirection=(char ***)realloc(cmd->redirection, sizeof(char **)*(cpt+1));
            cmd->redirectionType=(char ***)realloc(cmd->redirectionType, sizeof(char **)*(cpt+1));
        }
        getRedirection(&(cmd->redirection[cpt]), cmd->cmdMembers[cpt], &(cmd->redirectionType[cpt]));

        //Get cmdMembersArgs
        if(cmd->cmdMembersArgs==NULL) {
            cmd->cmdMembersArgs=(char ***)malloc(sizeof(char **));
            cmd->nbMembersArgs=(unsigned int *)malloc(sizeof(unsigned int));
        } else {
            cmd->cmdMembersArgs=(char ***)realloc(cmd->cmdMembersArgs, sizeof(char **)*(cpt+1));
            cmd->nbMembersArgs=(unsigned int *)realloc(cmd->nbMembersArgs, sizeof(unsigned int)*(cpt+1));
        }
        getMemberArg(&(cmd->cmdMembersArgs[cpt]), cmd->cmdMembers[cpt], &(cmd->nbMembersArgs[cpt]));

        //find next cmd (include blank)
        curIpt=index(curIpt, '|');
        if(curIpt!=NULL) {
            curIpt++;
        } else {
            break;
        }
   }
   for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
        if(cmd->redirection[cpt]==NULL) {
            //Format has error
            return 1;
        }
   }

   return 0;
}

/** \brief freeCmd
 * A function which frees memory associated to a command
 * \author Y. LIN
 * \param cmd *cmd: A pointer which points to the command
 * \return None
 *
 */
void freeCmd(cmd  *cmd){
  // Frees commands
  free(cmd->initCmd);
  unsigned int cpt;
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    free(cmd->cmdMembers[cpt]);
  }
  free(cmd->cmdMembers);
  // Frees commands's members' arguments
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    cmd->nbMembersArgs[cpt]++;
    while(cmd->nbMembersArgs[cpt]!=0) {
      free(cmd->cmdMembersArgs[cpt][--cmd->nbMembersArgs[cpt]]);
    }
    free(cmd->cmdMembersArgs[cpt]);
  }
  free(cmd->nbMembersArgs);
  free(cmd->cmdMembersArgs);
  // Frees commands's members' redirections
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    if(cmd->redirection[cpt] != NULL) {
      free(cmd->redirection[cpt][STDIN_FILENO]);
      free(cmd->redirection[cpt][STDOUT_FILENO]);
      free(cmd->redirection[cpt][STDERR_FILENO]);
    }
    free(cmd->redirection[cpt]);
  }
  free(cmd->redirection);
  // Frees commands's members' redirections' types
  for(cpt=0; cpt<cmd->nbCmdMembers; cpt++) {
    if(cmd->redirectionType[cpt] != NULL) {
      free(cmd->redirectionType[cpt][0]);
      free(cmd->redirectionType[cpt][1]);
    }
    free(cmd->redirectionType[cpt]);
  }
  free(cmd->redirectionType);

  cmd->nbCmdMembers=0;
}
