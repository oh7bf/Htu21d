#include "htu21d.h"

// write command byte to i2c address and wait delay in microseconds 
// one is returned is success and negtive number in case of failure
// in case of more serious failure the global integer 'cont' is set to zero 
int WriteCommand(int address, unsigned char cmd, int delay)
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
  cnt=I2LOCK_MAX;
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
  buf[0]=cmd; 
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

  usleep(delay);
  close(fd);

  cont=1;

  return 1;
}

