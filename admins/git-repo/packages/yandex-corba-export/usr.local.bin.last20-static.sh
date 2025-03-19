#!/bin/sh

count=0
MAXERRORS=10
TMPFILE=`mktemp`
STATICFILE="/usr/local/www5/export/last/last20x-static.xml"

curl -sfo $TMPFILE "http://export.`hostname -s`.yandex.ru/last/last20x.xml"
#LINES=`cat $TMPFILE | wc -l`
LINES=`grep -o '<item' $TMPFILE | wc -l`

while [ $LINES -lt 100 ] && [ $count -lt $MAXERRORS ];
do
	sleep 1;
	curl -sfo $TMPFILE "http://export.`hostname -s`.yandex.ru/last/last20x.xml"
	LINES=`cat $TMPFILE | wc -l`
	count=$[$count + 1];
done

[ $count -eq $MAXERRORS ] && exit 1;
mv $TMPFILE $STATICFILE
chmod 0644 $STATICFILE
chown www-data:www-data $STATICFILE
