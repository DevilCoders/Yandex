#!/bin/bash
#Load config
MONGO_PORT=27018
#end configs
rm -rf /var/tmp/mongo-backup-*
hn=$(hostname)
#detect update group
UPDATE_FROM=$(curl --connect-timeout  10 'http://c.yandex-team.ru/api-cached/get_host_tags/'${hn} 2>/dev/null | grep "update_mongo_from" | sed 's/update_mongo_from_//g')
if [ -n "$1" ]; then
    UPDATE_FROM="$1"
fi

if [ "$UPDATE_FROM" == "" ]; then
        echo "tag update_mongo_from not found and no argument passed. exit"
        exit 1;
fi
#Detect if this host master
q=$(mongo  --host 127.0.0.1 --port ${MONGO_PORT}  --quiet  --eval 'db.isMaster().ismaster' )
if [ $? -ne "0" ]; then
        echo "Something wrong, exiting"
        exit 1;
fi
if [ $q != "true" ]; then
        echo "this host is not master, exiting"
        exit 1;
fi


a=$(curl --connect-timeout  10 'http://c.yandex-team.ru/api-cached/groups2hosts/'${UPDATE_FROM}?'fields=fqdn' 2>/dev/null)
if [ "$a" == "" ]; then
        echo "Can't detect mongodb server, exiting"
        exit 1;
fi

failed=0
for i in $a; do
        s=$(mongo --ipv6 --host ${i} --port ${MONGO_PORT} --quiet --eval 'rs.conf().members.forEach(function(x){ if (x.hidden=='true')print(x.host) })') || failed=1
	
	if [ "$failed" -eq "0" ]; then
		s=$(echo "$s" | awk -F: {'print $1'})
		if [ $s ]; then 
			break; 
		fi
	fi
done

if [ "$failed" -eq "1" ]; then
	echo "Connection to all mongo nodes failed. Please check network hole."
	exit 1
fi

if [ "$s" == "" ]; then
        echo "Can't detect hidden backup host.  exiting"
        exit 1
fi
echo  "Backup Host is $s"
bdir="/var/tmp/mongo-backup-$$"
mkdir $bdir
echo "start backuping"
mongodump --ipv6 --host ${s} --port ${MONGO_PORT}  --out ${bdir}
if [ $? -ne "0" ]; then
        echo "Backup fail exiting"
        exit 1;
fi
echo "Restoring.."
mongorestore --drop --port ${MONGO_PORT}  $bdir
if [ $? -ne "0" ]; then
        echo "Restore fail"
        exit 1;
fi
#rm -rf ${bdir}
echo "All done"

