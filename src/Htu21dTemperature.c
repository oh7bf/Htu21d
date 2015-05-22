#include "htu21d.h"
#include "WriteCommandRead3.h"
#include "WriteFile.h"

double Htu21dTemperature(const char tdatafile[200])
{
  unsigned short T=0;
  unsigned int D=0;
  double temp=-100;
  char message[200]="";


  D = WriteCommandRead3(HTU21D_ADDRESS, HTU21D_TRIG_TEMP_MEAS, 50000);
  if( D == UINT_MAX )
  {
    syslog(LOG_INFO|LOG_DAEMON, "Reading temperature failed");
  }
  else
  {
    T = (unsigned short)(D>>8);
    T &= 0xFFFC;
    temp = -46.85+175.72*((double)T)/65536.0;
    sprintf(message,"Temperature %-+6.3f C",temp);
    syslog(LOG_DEBUG, "%s", message);
    WriteFile(tdatafile, temp);
  }

  return temp;
}


