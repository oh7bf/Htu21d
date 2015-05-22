#include "htu21d.h"

// write command byte to i2c address and read byte, which is returned as 
// unsigned short and USHRT_MAX in case of failure, in more serious failure
// global integer 'cont' is set to zero
unsigned short ReadRegister(int address, unsigned char cmd)
{
  unsigned short value=0;
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];

  char message[200]="";

  if( ( fd=open(i2cdev, O_RDWR) ) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to open i2c port");
    return USHRT_MAX;
  }
  rd = flock(fd, LOCK_EX|LOCK_NB);
  cnt = I2LOCK_MAX;
  while( (rd==1) && (cnt>0) ) // try again if port locking failed
  {
    sleep(1);
    rd=flock(fd, LOCK_EX|LOCK_NB);
    cnt--;
  }

  if(rd)
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to lock i2c port");
    close(fd);
    return USHRT_MAX;
  }

  cont=0;
  buf[0]=cmd; 
  if( ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Unable to get bus access to talk to slave");
    close(fd);
    return USHRT_MAX;
  }

  if( write(fd, buf, 1) != 1 ) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Error writing to i2c slave");
    close(fd);
    return USHRT_MAX;
  }
  else
  {
    if( read(fd, buf,1) != 1 ) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "Unable to read from slave");
      close(fd);
      return USHRT_MAX;
    }
    else 
    {
      value=(unsigned short)buf[0];
      sprintf(message, "Register 0x%02x", buf[0]);
      syslog(LOG_DEBUG, "%s", message);
      cont=1; 
    }
  }
  close(fd);

  return value;
}


