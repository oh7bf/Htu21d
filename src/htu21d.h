#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sqlite3.h>
#include <limits.h>

#ifndef HTU21D_H_INCLUDED
#define HTU21D_H_INCLUDED

#define HTU21D_ADDRESS 0x40
#define HTU21D_WRITE_USER_REG 0xE6
#define HTU21D_READ_USER_REG 0xE7
#define HTU21D_TRIG_TEMP_MEAS 0xF3
#define HTU21D_TRIG_HUM_MEAS 0xF5
#define HTU21D_SOFT_RESET 0xFE 

extern const char *i2cdev;// i2c device
extern const int i2lockmax; // maximum number of times to try lock i2c port  
extern int loglev; // log level
extern int cont; // main loop flag
extern char *dbfile; // SQLite database file name
#endif
