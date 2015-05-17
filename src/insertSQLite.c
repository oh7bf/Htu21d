#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <syslog.h>
#include <sqlite3.h>
#include "htu21d.h"

void insertSQLite(double t, double h)
{
  sqlite3 *db;
  sqlite3_stmt *stmt; 
  const char query[200]="insert into htu21d (temperature,humidity) values (?,?)";  
  char message[200]="";

  int rc;

  rc = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READWRITE, NULL);
  if( rc!=SQLITE_OK ){
    sprintf(message, "Can't open database: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    sqlite3_close(db);
    return;
  }

  rc = sqlite3_prepare_v2(db, query, 200, &stmt, 0);
  if( rc==SQLITE_OK )
  {
    rc = sqlite3_bind_double(stmt, 1, t);
    if( rc!=SQLITE_OK)
    {
      sprintf(message, "Binding failed: %s", sqlite3_errmsg(db));
      syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    }

    rc = sqlite3_bind_double(stmt, 2, h);
    if( rc!=SQLITE_OK)
    {
      sprintf(message, "Binding failed: %s", sqlite3_errmsg(db));
      syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    }

    rc = sqlite3_step(stmt); 
    if( rc!=SQLITE_DONE )// could be SQLITE_BUSY here 
    {
      sprintf(message, "Statement failed: %s", sqlite3_errmsg(db));
      syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    }
  }
  else
  {
    sprintf(message, "Statement prepare failed: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(db);

  return;
}

