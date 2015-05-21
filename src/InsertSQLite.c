#include "htu21d.h"

void InsertSQLite(const char query[200], int n, double data[10])
{
  sqlite3 *db;
  sqlite3_stmt *stmt; 
  char message[200]="";

  int rc;

  rc = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READWRITE, NULL);
  if( rc!=SQLITE_OK ){
    sprintf(message, "Can't open database: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    sqlite3_close(db);
    return;
  }

  int i;
  rc = sqlite3_prepare_v2(db, query, 200, &stmt, 0);
  if( rc==SQLITE_OK )
  {
    for(i=1;i<=n;i++)
    {
       rc = sqlite3_bind_double(stmt, i, data[i-1]);
       if( rc!=SQLITE_OK)
       {
         sprintf(message, "Binding failed: %s", sqlite3_errmsg(db));
         syslog(LOG_ERR|LOG_DAEMON, "%s", message);
       }
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

