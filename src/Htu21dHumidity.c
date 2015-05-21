#include "htu21d.h"
#include "WriteFile.h"

double Htu21dHumidity(const char hdatafile[200])
{
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];
  unsigned short H=0;
  double rhum=0;
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
  buf[0]=HTU21D_TRIG_HUM_MEAS;
  if(ioctl(fd, I2C_SLAVE, HTU21D_ADDRESS) < 0) 
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

