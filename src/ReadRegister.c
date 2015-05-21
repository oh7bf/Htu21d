#include "htu21d.h"

// write command byte to i2c address and read byte, which is returned as 
// !USE unsigned short here!
int ReadRegister(int address, unsigned char cmd)
{
  int value=0;
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];

  char message[200]="";

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON,"Failed to open i2c port");
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
    syslog(LOG_ERR|LOG_DAEMON,"Failed to lock i2c port");
    close(fd);
    return -2;
  }
  cont=0;
  buf[0]=cmd; 
  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON,"Unable to get bus access to talk to slave");
    close(fd);
    return -3;
  }
  if((write(fd, buf, 1)) != 1) 
  {
    syslog(LOG_ERR|LOG_DAEMON,"Error writing to i2c slave");
    close(fd);
    return -4;
  }
  else
  {
    if(read(fd, buf,1) != 1) 
    {
      syslog(LOG_ERR|LOG_DAEMON,"Unable to read from slave");
      close(fd);
      return -5;
    }
    else 
    {
      value=(int)buf[0];
      sprintf(message,"Register 0x%02x",buf[0]);
      syslog(LOG_DEBUG, "%s", message);
      cont=1; 
    }
  }
  close(fd);

  return value;
}


