#!/bin/bash
# i use this documentation: haproxy.1wt.eu/download/1.4/doc/configuration.txt
# 9.2.      Unix Socket commands

ggg=$1;

# k - server or backend, 1 for frontends, 2 for backends, 4 for servers, -1 for everything; d is field where is the name of server/backend

case "$ggg" in
    servers|server)
     k=4;
     d=2;
    ;;
    backends|backend)
     k=2;
     d=1;
    ;;

    *)
        echo "2; i don't know this option \`$1'" >&2
        exit 1
    ;;
esac;

down_servers='';
#Ger id's of backends, then get status of all servers in that backend or status of backend, then get the name of down server/backend.
for i in `cat /etc/haproxy/haproxy.cfg | grep id | grep -v '#' | awk '{print $2}' ` ; do
    down_servers=`echo "show stat $i $k -1" |  socat unix-connect:/tmp/haproxy-tools  stdio | grep DOWN  | cut -d',' -f${d} ; echo $down_servers` ;
 done
# remove \n, because command echo add it to each line.
down_servers=`echo "$down_servers" | tr '\n' ' '`
#echo $down_servers;
if [ "$down_servers" = " " ] ; then echo "0; Ok" ; else echo "2; $down_servers is down, look at `hostname -f`:1936"; fi




