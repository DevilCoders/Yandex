#!/bin/sh
#
# Check ping local MySQL
#
# $Id: mysql_ping.sh,v 1.5 2008/04/08 15:54:30 andozer Exp $
#
#me=${0##*/}# strip path
#me=${me%.*}# strip extension
BASE=$HOME/agents
#CONF=$BASE/etc/$me.conf
PREFIX=/usr/local
DB_ADM="mysqladmin"

TSTAMP=`date '+%s'`

MYSQL_BIN="$PREFIX/mysql/bin"
DB_USER=monitor
DB_PASS=uchae6Ro
DB_HOST=`hostname -f`


#[ -s $CONF ] && . $CONF

PATH="/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$MYSQL_BIN"

die () {
err_c="$1"
shift
echo "$err_c;$*"
exit 0
}

(which $DB_ADM > /dev/null) || die 2 "Can't stat $DB_ADM"

result=`$DB_ADM ${DB_HOST:+-h "$DB_HOST"} ${DB_USER:+-u "$DB_USER"} \
${DB_PASS:+-p"$DB_PASS"} ping 2>&1  | tr '\n' ' '`

if [ $? -eq 0 ]
then
err_c=0
else
err_c=2
fi

die $err_c "$result"
