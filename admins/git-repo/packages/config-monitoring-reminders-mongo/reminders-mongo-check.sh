#!/bin/bash

#check installed packages
dpkg -l | grep yandex-reminders-api | grep ii 2>&1>/dev/null;
ins_api=$?
if [ $ins_api -eq 0  ] ; then
H='reminders-api'; ans_api=`curl -sm3 http://localhost:81/ping-mongo -H "Host: ${H}.*"`; fi

dpkg -l | grep yandex-reminders-worker | grep ii 2>&1>/dev/null;
ins_work=$?
#if $ins_work ; then echo ins ; else echo not ins ; fi
if [ $ins_work -eq 0  ] ; then
H='reminders-worker'; ans_work=`curl -sm3 http://localhost:81/ping-mongo -H "Host: ${H}.*"`; fi


if [ $ins_work -eq 0 ] && [ $ins_api -eq 0 ] ; then
 ins_all=0 ;
 if [ "${ans_api}" = "ok /ping-mongo" ] && [ "${ans_work}" = "ok /ping-mongo" ] ; then
  echo "0; ok" ;
 else
  if [ "${ans_api}" = "ok /ping-mongo" ] ; then
    echo "2; try curl -sm3 http://localhost:81/ping-mongo -H \"Host: reminders-worker.*\"" ;
  else
    echo "2; try curl -sm3 http://localhost:81/ping-mongo -H \"Host: reminders-api.*\"";
  fi

 fi
else
  ins_all=1 ;
  if [ "${ans_api}" = "ok /ping-mongo" ] || [ "${ans_work}" = "ok /ping-mongo" ] ; then
    echo "0; ok" ;
  else
    echo "2; try curl -sm3 http://localhost:81/ping-mongo -H \"Host: ${H}.*\"";
  fi
fi


