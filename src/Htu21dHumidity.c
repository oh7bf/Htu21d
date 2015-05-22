#include "htu21d.h"
#include "WriteCommandRead3.h"
#include "WriteFile.h"

double Htu21dHumidity(const char hdatafile[200])
{
  unsigned short H=0;
  unsigned int D=0;
  double rhum=-100;
  char message[200]="";

  D = WriteCommandRead3(HTU21D_ADDRESS, HTU21D_TRIG_HUM_MEAS, 16000);
  if( D == UINT_MAX )
  {
    syslog(LOG_INFO|LOG_DAEMON, "Reading humidity failed");
  }
  else
  {
    H = (unsigned short)(D>>8);
    H &= 0xFFFC;
    rhum = -6+125*((double)H)/65536.0;
    sprintf(message,"Humidity %-5.1f %%",rhum);
    syslog(LOG_DEBUG, "%s", message);
    WriteFile(hdatafile, rhum);
  }

  return rhum;
}

