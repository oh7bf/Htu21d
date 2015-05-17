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
#include <syslog.h>
#include "htu21d.h"

int resetchip()
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to open i2c port");
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
    syslog(LOG_ERR|LOG_DAEMON, "Failed to open i2c port");
    close(fd);
    return -2;
  }

  cont=0;
  buf[0]=0xfe; // soft reset command 
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Unable to get bus access to talk to slave");
    close(fd);
    return -3;
  }

  if((write(fd, buf, 1)) != 1) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Error writing to i2c slave");
    close(fd);
    return -4;
  }

  usleep(20000);
  close(fd);

  cont=1;

  return 1;
}

