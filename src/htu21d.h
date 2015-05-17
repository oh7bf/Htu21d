#ifndef HTU21D_H_INCLUDED
#define HTU21D_H_INCLUDED
extern const char *i2cdev;// i2c device
extern const int address; // i2c address
extern const int i2lockmax; // maximum number of times to try lock i2c port  
extern int loglev; // log level
extern int cont; // main loop flag
extern char *dbfile; // SQLite database file name
#endif

