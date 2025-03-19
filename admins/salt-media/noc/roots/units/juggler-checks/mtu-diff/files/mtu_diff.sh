#!/bin/bash

. /usr/local/sbin/autodetect_active_eth

ifconfig=`which ifconfig`

func_mtu_diff(){
  find_1500_mtu=""
  err_i=""
  n="0"
  
  if [ "x$default_bridge" = "x" ] ; then
      find_1500_mtu=`$ifconfig $default_iface | sed 's/://g' | head -n 1 |awk '{if ($NF == 1500) print $1}'`;
  fi

  for i in `brctl show |egrep '^(fb|br)[[:digit:]]' |awk '{print $1}'`; do
    for t in `ls -1 /sys/devices/virtual/net/"$i"/brif |egrep -v 'eth[0-9]$'`; do
      $ifconfig $t | head -n 1 | awk '{print $NF}' >>/tmp/"$i".tmp
      find_1500_mtu=$find_1500_mtu`$ifconfig $t | sed 's/://g' | head -n 1 | awk '{if ($NF == 1500) print $1","}'`
    done
    if [ -f /tmp/"$i".tmp ]; then
      if [ `cat /tmp/"$i".tmp |sort |uniq |wc -l` -ne 1 ]; then
        n=$(($n + 1));
        err_i=$err_i$i;
      fi
    fi
  done
  
  if [ -z "$find_1500_mtu" ]; then
    if [ $n -eq 0 ]; then
      echo "PASSIVE-CHECK:mtu-diff;0;OK"
    else
      echo "PASSIVE-CHECK:mtu-diff;2;Found different MTU for $err_i";
    fi
  else
    echo "PASSIVE-CHECK:mtu-diff;2;Found default MTU ($find_1500_mtu). AHTUNG"
  fi
}

func_mtu_diff;
