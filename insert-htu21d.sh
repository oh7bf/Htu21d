#!/bin/sh

temperature=0
humidity=0

read temperature < /var/lib/htu21d/temperature
read humidity < /var/lib/htu21d/humidity

/usr/bin/sqlite3 /var/lib/htu21d/htu21d.db "insert into htu21d \
(temperature,humidity) values (\"$temperature\",\"$humidity\");"

