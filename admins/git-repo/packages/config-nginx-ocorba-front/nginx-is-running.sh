#!/bin/bash
count=`ps aux | grep 'nginx: master' | grep -vc grep` ;
if [ $count -eq 1 ] ; then echo "0; ok" ; else echo "2; nginx not running" ; fi
