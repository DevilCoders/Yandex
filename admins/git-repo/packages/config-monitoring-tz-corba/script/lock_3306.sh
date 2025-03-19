#!/bin/sh
#
# Check locking for MySQL's system user
#
# $Id: lock.sh,v 1.14 2005/06/24 07:53:36 gvs Exp $
#
me=${0##*/}	# strip path
me=${me%.*}	# strip extension
BASE=$HOME/agents
#CONF=$BASE/etc/$me.conf
PREFIX=/usr/local
MYSQL=mysql
DESCR="MySQL Lock"
TMPDIR=/tmp
TMP=$BASE/tmp
FILE=$TMP/$me

DB_USER=monitor
DB_PASS=uchae6Ro
DB_SOCKET=/var/run/mysqld/mysqld.sock

#[ -s $CONF ] && . $CONF

PATH="/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$PREFIX/$MYSQL/bin"

DB_ADM=${DB_ADM:-mysql}
DB_SOCKET=${DB_SOCKET:-$TMPDIR/$MYSQL.sock}
DB_USER=${DB_USER:-root}
CHECK_LOGPOS=${CHECK_LOGPOS:-0}

die () {

	echo "$1;$2"
	exit 0
}
set `$DB_ADM -u $DB_USER -S $DB_SOCKET --password="$DB_PASS" \
-e "SHOW SLAVE STATUS\G" | awk -F ":" -v check_logpos="$CHECK_LOGPOS" '
BEGIN { slave_io=0; slave_sql=0; curlogpos=0;
}
	($1 ~ /Slave_IO_Running/) {
		if ($2 ~ /Yes/) { slave_io=1; }
		next;
	}
	($1 ~ /Slave_SQL_Running/) {
		if ($2 ~ /Yes/) { slave_sql=1; }
		next;
	}
	(check_logpos==1) {
		if ($1 ~ /Read_Master_Log_Pos/) {
			curlogpos=$2;
		}
	}
END {
	print slave_io, slave_sql, curlogpos;
}'`

[ $# -ne 3 ] && die 2 "SHOW SLAVE STATUS command execution failure or bad output"
IO=$1
SQL=$2
CURPOS=$3


[ $IO -eq 1 -a $SQL -eq 1 ] || die 2 "Slave is not running"
if [ $CHECK_LOGPOS -eq 1 ] 
then
	[ -s $FILE ] && PREVPOS=`cat $FILE`
	PREVPOS=${PREVPOS:-0}
	echo $CURPOS > $FILE
	#echo prev $PREVPOS cur $CURPOS
	[ $PREVPOS -eq $CURPOS ] && die 2 "Read_Master_Log_Pos is stale"
fi
die 0 "Ok"

exit

