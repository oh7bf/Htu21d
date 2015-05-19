/**************************************************************************
 * 
 * Read humidity from HTU21D chip with I2C and write it to a file and 
 * database. 
 *       
 * Copyright (C) 2014 - 2015 Jaakko Koivuniemi.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 *
 * Sun Nov 30 18:48:05 CET 2014
 * Edit: Tue May 19 21:08:08 CEST 2015
 *
 * Jaakko Koivuniemi
 **/

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include "htu21d.h"
#include "InsertSQLite.h"
#include "ReadSQLiteTime.h"
#include "ReadRegister.h"
#include "WriteCommand.h"
#include "WriteFile.h"
#include "Htu21dTemperature.h"

const int version=20150519; // program version
int measint=300; // measurement interval [s]
int softreset=0; // 1=soft reset at start

double temperature=0; // temperature [C]
double humidity=0; // humidity [%]
const char hdatafile[200]="/var/lib/htu21d/humidity";
const char tdatafile[200]="/var/lib/htu21d/temperature";
const char query[200]="insert into htu21d (temperature,humidity) values (?,?)";

// local SQLite database file
int dbsqlite=0; // store data to local SQLite database
char *dbfile=NULL;

const char *i2cdev="/dev/i2c-1";
const int  address=0x40;
const int  i2lockmax=10; // maximum number of times to try lock i2c port  

const char confile[200]="/etc/htu21d_config";

const char pidfile[200]="/var/run/htu21d.pid";

int loglev=5;
char message[200]="";

void read_config()
{
  FILE *cfile;
  char *line=NULL;
  char par[20];
  float value;
  size_t len;
  ssize_t read;

  line = malloc(sizeof(char)*(200));
  dbfile = malloc(sizeof(char)*(200));

  cfile=fopen(confile, "r");
  if(NULL!=cfile)
  {
    syslog(LOG_INFO|LOG_DAEMON, "Read configuration file");

    while((read=getline(&line,&len,cfile))!=-1)
    {
       if(sscanf(line,"%s %f",par,&value)!=EOF) 
       {
          if(strncmp(par,"LOGLEVEL",8)==0)
          {
             loglev=(int)value;
             sprintf(message,"Log level set to %d",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
             setlogmask(LOG_UPTO (loglev));
          }
          if(strncmp(par,"DBSQLITE",8)==0)
          {
            if(sscanf(line,"%s %s",par,dbfile)!=EOF)  
            {
              dbsqlite=1;
              sprintf(message, "Store data to database %s", dbfile);
              syslog(LOG_INFO|LOG_DAEMON, "%s", message);
            }
          }
          if(strncmp(par,"SOFTRESET",9)==0)
          {
             if(value==1)
             {
               softreset=1;
               syslog(LOG_INFO|LOG_DAEMON, "Soft reset at start");
             }
          }
          if(strncmp(par,"MEASINT",7)==0)
          {
             measint=(int)value;
             sprintf(message,"Measurement interval set to %d s",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
          }
       }
    }
    fclose(cfile);
  }
  else
  {
    sprintf(message, "Could not open %s", confile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }
}

int cont=1; /* main loop flag */


// read humidity from chip
double read_humidity()
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];
  unsigned short H=0;
  double rhum=0;

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to open i2c port");
    return -100;
  }
  rd=flock(fd, LOCK_EX|LOCK_NB);
  cnt=i2lockmax;
  while((rd==1)&&(cnt>0)) // try again if port locking failed
  {
    sleep(1);
    rd=flock(fd, LOCK_EX|LOCK_NB);
    cnt--;
  }
  if(rd)
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to lock i2c port");
    close(fd);
    return -200;
  }

  cont=0;
  buf[0]=0xf5;
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Unable to get bus access to talk to slave");
    close(fd);
    return -300;
  }
  if((write(fd, buf, 1)) != 1) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Error writing to i2c slave");
    close(fd);
    return -400;
  }
  else
  {
    usleep(16000);
    if(read(fd, buf, 3) != 3) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "Unable to read from slave");
      close(fd);
      return -500;
    }
    else 
    {
      sprintf(message,"read 0x%02x%02x%02x\n",buf[0],buf[1],buf[2]);
      syslog(LOG_DEBUG, "%s", message);
      H=((unsigned short)buf[0])<<8;
      H|=(unsigned short)buf[1];
      H&=0xFFFC;
      rhum=-6+125*((double)H)/65536.0;
      sprintf(message,"Humidity %-5.1f %%",rhum);
      syslog(LOG_DEBUG, "%s", message);

      cont=1;
      WriteFile(hdatafile, rhum);
    }
  }
  close(fd);

  return rhum;
}


void stop(int sig)
{
  sprintf(message,"signal %d catched, stop",sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);
  cont=0;
}

void terminate(int sig)
{
  sprintf(message,"signal %d catched",sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);

  sleep(1);
  syslog(LOG_NOTICE|LOG_DAEMON, "stop");

  cont=0;
}

void hup(int sig)
{
  sprintf(message,"signal %d catched",sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);
}


int main()
{  
  int ok=0;

  setlogmask(LOG_UPTO (loglev));
  syslog(LOG_NOTICE|LOG_DAEMON, "htu21d v. %d started", version); 

  signal(SIGINT,&stop); 
  signal(SIGKILL,&stop); 
  signal(SIGTERM,&terminate); 
  signal(SIGQUIT,&stop); 
  signal(SIGHUP,&hup); 

  read_config();

  int unxs=(int)time(NULL); // unix seconds
  int nxtmeas=unxs; // next time to read temperature and humidity

  pid_t pid, sid;
        
  pid=fork();
  if(pid<0) 
  {
    exit(EXIT_FAILURE);
  }

  if(pid>0) 
  {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  /* Create a new SID for the child process */
  sid=setsid();
  if(sid<0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "failed to create child process");
    exit(EXIT_FAILURE);
  }
        
  if((chdir("/")) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "failed to change to root directory");
    exit(EXIT_FAILURE);
  }
        
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  FILE *pidf;
  pidf=fopen(pidfile,"w");

  if(pidf==NULL)
  {
    sprintf(message,"Could not open PID lock file %s, exiting", pidfile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    exit(EXIT_FAILURE);
  }

  if(flock(fileno(pidf),LOCK_EX||LOCK_NB)==-1)
  {
    sprintf(message,"Could not lock PID lock file %s, exiting", pidfile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    exit(EXIT_FAILURE);
  }

  fprintf(pidf,"%d\n",getpid());
  fclose(pidf);

  if( ReadRegister(address, 0xE7)<0 )
  {
    sprintf(message,"Failed to read user register, exit");
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    exit(EXIT_FAILURE);
  }
  else 
  {
    if(softreset==1) 
    {
      if( WriteCommand(address, 0xFE, 20000) != 1)
      {
        sprintf(message,"Chip reset failed");
        syslog(LOG_ERR|LOG_DAEMON, "%s", message);
        cont=0;
      }
    }
  }

  if(dbsqlite==1)
  {
    if(ReadSQLiteTime()==0) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "SQLite database read failed, drop database connection");
      dbsqlite=0; 
    }
  }

  double data[10];
  while(cont==1)
  {
    unxs=(int)time(NULL); 

    if((unxs>=nxtmeas)||((nxtmeas-unxs)>measint)) 
    {
      nxtmeas = measint+unxs;
      temperature = Htu21dTemperature(tdatafile);
      if(cont==1)
      {
        humidity=read_humidity();
        if(cont==1)
        {
          sprintf(message,"T=%-+6.3f C RH=%-5.1f %%",temperature,humidity);
          syslog(LOG_INFO|LOG_DAEMON, message);
          if(dbsqlite==1)
          {
             data[0]=temperature;
             data[1]=humidity; 
             InsertSQLite(query, 2, data);
          }
        }
      }
    }

    sleep(1);
  }

  syslog(LOG_NOTICE|LOG_DAEMON, "remove PID file");
  ok=remove(pidfile);

  return ok;
}
