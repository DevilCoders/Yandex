#/bin/bash
rm -rv /var/tmp/dnet_recovery_merge
dnet_recovery -o `hostname -f`:1025:10 -n 4 -l /dev/null -u -g `cat /etc/elliptics/group.id` merge
touch /run/lock/elliptics_merge.lock
