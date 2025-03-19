#!/bin/bash

case $1 in
	mount)
		if ! mount|grep ^arc > /dev/null; then
			echo "2; Arc is not mounted"
			exit 1
		fi
	;;

	update)
		MODIFY_TS=$(stat --format %Y /var/cache/salt/arc2salt.json)
		LAG=$(($(date +%s) - $MODIFY_TS))
		
		if [ $LAG -gt 3600 ];then
			echo "2; arc2salt is out of sync: last update" $(($LAG/3600)) "hours ago"
			exit 1
		fi
	;;
esac	

echo "0; Ok"
