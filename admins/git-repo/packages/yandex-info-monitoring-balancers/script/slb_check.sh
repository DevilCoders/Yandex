#!/bin/bash
if [ -z "$1" ] ;
  then
    echo "Usage: $0 conductor_group" ;
    exit 0 ;
fi
STATUS=0
DESC=''

BFHU=/tmp/balancers.${CG}.uniq
cat $BFHU | while read ip port type name ; do
    /usr/bin/jport.sh -s $ip -p $port >/dev/null 2>&1 ;
    EC=$?;
    if [ $EC -ne 0 ] ; then DESC=$DESC+'$ip fail' ; STATUS=2 ; fi
; done
echo $DESC ;


#!/bin/bash
if [ -z "$1" ] ;
  then
    echo "Usage: $0 conductor_group" ;
    exit 0 ;
fi
STATUS=0
D=''
CG=$1
BFHU=/tmp/balancers.${CG}.uniq
DESC=`cat $BFHU | while read ip port type name; do
    /usr/bin/jport.sh -s $ip -p $port >/dev/null 2>&1 ;
    EC=$?;
    if [ $EC -ne 0 ] ; then echo " $ip fail" ; STATUS=2 ; fi ;
# echo $DESC
done;`
#echo $DESC ;
if [ -z DESC ] ; then STATUS=0 ; else STATUS=2 ; fi ;
echo "$STATUS;$DESC"

