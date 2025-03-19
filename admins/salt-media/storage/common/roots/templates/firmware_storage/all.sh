#!/bin/bash

board=`/usr/local/lsi/sas2flash -list | grep 'Board Name' | grep -Po '(?<=SAS)\d{4}'`

case "$board" in
	"9200" ) /usr/local/lsi/9200.sh;;
	"9205" ) /usr/local/lsi/9205.sh;;
	"9207" ) /usr/local/lsi/9207.sh;;
esac
