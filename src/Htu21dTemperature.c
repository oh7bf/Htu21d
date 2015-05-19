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
#include <signal.h>
#include <syslog.h>
#include "htu21d.h"
#include "WriteFile.h"

double Htu21dTemperature(const char tdatafile[200])
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];
  unsigned short T=0;
  double temp=0;
  char message[200]="";

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
  buf[0]=0xf3;
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
    usleep(50000);
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
      T=((unsigned short)buf[0])<<8;
      T|=(unsigned short)buf[1];
      T&=0xFFFC;
      temp=-46.85+175.72*((double)T)/65536.0;
      sprintf(message,"Temperature %-+6.3f C",temp);
      syslog(LOG_DEBUG, "%s", message);
      cont=1;
      WriteFile(tdatafile, temp);
    }
  }
  close(fd);

  return temp;
}


