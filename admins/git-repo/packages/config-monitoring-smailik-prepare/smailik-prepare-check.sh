#!/bin/sh
C=`ps aux | grep smailik-preparer.pl | grep -v grep | wc -l`
if [ $C -eq 1 ] ; then echo "0;ok" ; else echo "2;May be smailik-preparer.pl not running" ; fi

