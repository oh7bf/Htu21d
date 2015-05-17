#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <syslog.h>
#include "htu21d.h"

void writefile(const char fname[200], double value)
{
  char message[200]=""; 
  FILE *tfile;
  tfile=fopen(fname, "w");
  if(NULL==tfile)
  {
    sprintf(message,"could not write to file: %s",fname);
    syslog(LOG_ERR|LOG_DAEMON, message);
  }
  else
  { 
    fprintf(tfile,"%-6.3f",value);
    fclose(tfile);
  }
}


