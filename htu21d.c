/**************************************************************************
 * 
 * Read humidity from HTU21D chip with I2C and write it to a log file. 
 *       
 * Copyright (C) 2014 Jaakko Koivuniemi.
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
 * Edit: Fri Dec 26 09:05:02 CET 2014
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

const int version=20141226; // program version
int measint=300; // measurement interval [s]
int softreset=0; // 1=soft reset at start

double temperature=0; // temperature [C]
double humidity=0; // humidity [%]
const char hdatafile[200]="/var/lib/htu21d/humidity";
const char tdatafile[200]="/var/lib/htu21d/temperature";

const char i2cdev[100]="/dev/i2c-1";
const int  address=0x40;
const int  i2lockmax=10; // maximum number of times to try lock i2c port  

const char confile[200]="/etc/htu21d_config";

const char pidfile[200]="/var/run/htu21d.pid";

int loglev=3;
const char logfile[200]="/var/log/htu21d.log";
char message[200]="";

void logmessage(const char logfile[200], const char message[200], int loglev, int msglev)
{
  time_t now;
  char tstr[25];
  struct tm* tm_info;
  FILE *log;

  time(&now);
  tm_info=localtime(&now);
  strftime(tstr,25,"%Y-%m-%d %H:%M:%S",tm_info);
  if(msglev>=loglev)
  {
    log=fopen(logfile, "a");
    if(NULL==log)
    {
      perror("could not open log file");
    }
    else
    { 
      fprintf(log,"%s ",tstr);
      fprintf(log,"%s\n",message);
      fclose(log);
    }
  }
}

void read_config()
{
  FILE *cfile;
  char *line=NULL;
  char par[20];
  float value;
  size_t len;
  ssize_t read;

  cfile=fopen(confile, "r");
  if(NULL!=cfile)
  {
    sprintf(message,"Read configuration file");
    logmessage(logfile,message,loglev,4);

    while((read=getline(&line,&len,cfile))!=-1)
    {
       if(sscanf(line,"%s %f",par,&value)!=EOF) 
       {
          if(strncmp(par,"LOGLEVEL",8)==0)
          {
             loglev=(int)value;
             sprintf(message,"Log level set to %d",(int)value);
             logmessage(logfile,message,loglev,4);
          }
          if(strncmp(par,"SOFTRESET",9)==0)
          {
             if(value==1)
             {
               softreset=1;
               sprintf(message,"Soft reset at start");
               logmessage(logfile,message,loglev,4);
             }
          }
          if(strncmp(par,"MEASINT",7)==0)
          {
             measint=(int)value;
             sprintf(message,"Measurement interval set to %d s",(int)value);
             logmessage(logfile,message,loglev,4);
          }
       }
    }
    fclose(cfile);
  }
  else
  {
    sprintf(message, "Could not open %s", confile);
    logmessage(logfile, message, loglev,4);
  }
}

int cont=1; /* main loop flag */

// send soft reset command
int resetchip()
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    sprintf(message,"Failed to open i2c port");
    logmessage(logfile,message,loglev,4);
    return -1;
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
    sprintf(message,"Failed to lock i2c port");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -2;
  }

  cont=0;
  buf[0]=0xfe; // soft reset command 
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    sprintf(message,"Unable to get bus access to talk to slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -3;
  }
  if((write(fd, buf, 1)) != 1) 
  {
    sprintf(message,"Error writing to i2c slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -4;
  }
  usleep(20000);
  close(fd);
  cont=1;

  return 1;
}

int read_register()
{
  int value=0;
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];
  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    sprintf(message,"Failed to open i2c port");
    logmessage(logfile,message,loglev,4);
    return -1;
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
    sprintf(message,"Failed to lock i2c port");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -2;
  }
  cont=0;
  buf[0]=0xe7; // read user register
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    sprintf(message,"Unable to get bus access to talk to slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -3;
  }
  if((write(fd, buf, 1)) != 1) 
  {
    sprintf(message,"Error writing to i2c slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -4;
  }
  else
  {
    if(read(fd, buf,1) != 1) 
    {
      sprintf(message,"Unable to read from slave");
      logmessage(logfile,message,loglev,4);
      close(fd);
      return -5;
    }
    else 
    {
      value=(int)buf[0];
      sprintf(message,"User register 0x%02x",buf[0]);
      logmessage(logfile,message,loglev,4);
      cont=1; 
    }
  }
  close(fd);

  return value;
}

// write temperature value to file
void write_temp(double t)
{
  FILE *tfile;
  tfile=fopen(tdatafile, "w");
  if(NULL==tfile)
  {
    sprintf(message,"could not write to file: %s",tdatafile);
    logmessage(logfile,message,loglev,4);
  }
  else
  { 
    fprintf(tfile,"%-+6.3f",t);
    fclose(tfile);
  }
}

// read temperature from chip
double read_temperature()
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];
  unsigned short T=0;
  double temp=0;

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    sprintf(message,"Failed to open i2c port");
    logmessage(logfile,message,loglev,4);
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
    sprintf(message,"Failed to lock i2c port");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -200;
  }

  cont=0;
  buf[0]=0xf3;
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    sprintf(message,"Unable to get bus access to talk to slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -300;
  }

  if((write(fd, buf, 1)) != 1) 
  {
    sprintf(message,"Error writing to i2c slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -400;
  }
  else
  {
    usleep(50000);
    if(read(fd, buf, 3) != 3) 
    {
      sprintf(message,"Unable to read from slave");
      logmessage(logfile,message,loglev,4);
      close(fd);
      return -500;
    }
    else 
    {
      sprintf(message,"read 0x%02x%02x%02x\n",buf[0],buf[1],buf[2]);
      logmessage(logfile,message,loglev,2);
      T=((unsigned short)buf[0])<<8;
      T|=(unsigned short)buf[1];
      T&=0xFFFC;
      temp=-46.85+175.72*((double)T)/65536.0;
      sprintf(message,"Temperature %-+6.3f C",temp);
      logmessage(logfile,message,loglev,2);
      cont=1;
      write_temp(temp); 
    }
  }
  close(fd);

  return temp;
}

// write humidity value to file
void write_humidity(double t)
{
  FILE *hfile;
  hfile=fopen(hdatafile, "w");
  if(NULL==hfile)
  {
    sprintf(message,"could not write to file: %s",hdatafile);
    logmessage(logfile,message,loglev,4);
  }
  else
  { 
    fprintf(hfile,"%-5.1f",t);
    fclose(hfile);
  }
}

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
    sprintf(message,"Failed to open i2c port");
    logmessage(logfile,message,loglev,4);
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
    sprintf(message,"Failed to lock i2c port");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -200;
  }

  cont=0;
  buf[0]=0xf5;
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    sprintf(message,"Unable to get bus access to talk to slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -300;
  }
  if((write(fd, buf, 1)) != 1) 
  {
    sprintf(message,"Error writing to i2c slave");
    logmessage(logfile,message,loglev,4);
    close(fd);
    return -400;
  }
  else
  {
    usleep(16000);
    if(read(fd, buf, 3) != 3) 
    {
      sprintf(message,"Unable to read from slave");
      logmessage(logfile,message,loglev,4);
      close(fd);
      return -500;
    }
    else 
    {
      sprintf(message,"read 0x%02x%02x%02x\n",buf[0],buf[1],buf[2]);
      logmessage(logfile,message,loglev,2);
      H=((unsigned short)buf[0])<<8;
      H|=(unsigned short)buf[1];
      H&=0xFFFC;
      rhum=-6+125*((double)H)/65536.0;
      sprintf(message,"Humidity %-5.1f %%",rhum);
      logmessage(logfile,message,loglev,2);

      cont=1;
      write_humidity(rhum); 
    }
  }
  close(fd);

  return rhum;
}


void stop(int sig)
{
  sprintf(message,"signal %d catched, stop",sig);
  logmessage(logfile,message,loglev,4);
  cont=0;
}

void terminate(int sig)
{
  sprintf(message,"signal %d catched",sig);
  logmessage(logfile,message,loglev,4);

  sleep(1);
  strcpy(message,"stop");
  logmessage(logfile,message,loglev,4);

  cont=0;
}

void hup(int sig)
{
  sprintf(message,"signal %d catched",sig);
  logmessage(logfile,message,loglev,4);
}


int main()
{  
  int ok=0;

  sprintf(message,"htu21d v. %d started",version); 
  logmessage(logfile,message,loglev,4);

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
    strcpy(message,"failed to create child process"); 
    logmessage(logfile,message,loglev,4);
    exit(EXIT_FAILURE);
  }
        
  if((chdir("/")) < 0) 
  {
    strcpy(message,"failed to change to root directory"); 
    logmessage(logfile,message,loglev,4);
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
    logmessage(logfile,message,loglev,4);
    exit(EXIT_FAILURE);
  }

  if(flock(fileno(pidf),LOCK_EX||LOCK_NB)==-1)
  {
    sprintf(message,"Could not lock PID lock file %s, exiting", pidfile);
    logmessage(logfile,message,loglev,4);
    exit(EXIT_FAILURE);
  }

  fprintf(pidf,"%d\n",getpid());
  fclose(pidf);

  if(read_register()<0)
  {
    sprintf(message,"Failed to read user register, exit");
    logmessage(logfile,message,loglev,4);
    exit(EXIT_FAILURE);
  }
  else 
  {
    if(softreset==1) 
    {
      if(resetchip()!=1)
      {
        sprintf(message,"Chip reset failed");
        logmessage(logfile,message,loglev,4);
        cont=0;
      }
    }
  }

  while(cont==1)
  {
    unxs=(int)time(NULL); 

    if((unxs>=nxtmeas)||((nxtmeas-unxs)>measint)) 
    {
      nxtmeas=measint+unxs;
      temperature=read_temperature();
      if(cont==1)
      {
        humidity=read_humidity();
        if(cont==1)
        {
          sprintf(message,"T=%-+6.3f C RH=%-5.1f %%",temperature,humidity);
          logmessage(logfile,message,loglev,4);
// optional script to insert the data to local database
//          ok=system("/usr/sbin/insert-htu21d.sh");
        }
      }
    }

    sleep(1);
  }

  strcpy(message,"remove PID file");
  logmessage(logfile,message,loglev,4);
  ok=remove(pidfile);

  return ok;
}
