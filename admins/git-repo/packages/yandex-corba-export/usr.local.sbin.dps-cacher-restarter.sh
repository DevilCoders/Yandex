#!/bin/sh

h=`hostname -f`
rnd=`host $h |rev|cut -f1 -d'.'|rev`
sec=$[$rnd*15]
echo DPS Cacher restarter on $h. $sec seconds to wait.
sleep $sec
/etc/init.d/CORBA restart dps-cacher
