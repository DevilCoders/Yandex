#!/bin/bash

CF3DIR=/home/cf/cf3
mkdir -p /home/cf/cf3/tmp
CFDB=cfdb-dbs.kp.yandex.net
cd $CF3DIR

while ! mkdir /tmp/cf.lock &>/dev/null ; do sleep 0.5 ; done
trap 'rm -r /tmp/cf.lock' EXIT
UIDS=`echo $1|perl -pe 's/[^0-9,]//g;s/^([^0-9]+)//g;s/([^0-9]+)$//'`
#HOSTS="cf@cfcalc:8"
CFCALC=`curl -s http://c.yandex-team.ru/api-cached/groups2hosts/kp-cfcalc |sed -E "s/(.*)/\1:7/"`
HOSTS=$CFCALC

N=0

mysql -h $CFDB -N kinopoisk  -e 'SELECT 10000000000*user_id+id, v.user_id,v.id, v.vote FROM kinopoisk.users_rel_vote v WHERE v.vote>0 AND v.user_id in ('$UIDS') order by v.user_id,v.id;' > urv-users.csv
USERS=`sort -u -n -m  urv.fix.csv  urv-users.csv | awk '{print $2,"\t",$3,"\t",$4}' | ./prepare_cf3`
rm -f urv-users.csv urv1.csv
echo $UIDS|perl -pe 's/,/\n/g'|sort -nu > userlist.txt

for h in $HOSTS ; do
    N=$(($N + `echo $h |cut -d: -f 2 ` ))
    HOST=`echo $h |cut -d: -f 1 `
    echo "done"
done
wait
CUR=1
for h in $HOSTS ; do
    HOST=`echo $h |cut -d: -f 1 `
    HN=`echo $h |cut -d: -f 2 `
    for i in `seq 1 $HN` ; do
	$CF3DIR/cf3.sh -u $USERS -p $N -c $CUR -i userlist.txt > cf3.$CUR.result &
	CUR=$(( $CUR +1 ))
    done
done

wait
for i in `seq 1 $N` ; do
  sort -nr -T $CF3DIR/tmp/ cf3.$i.result > s.cf3.$i.result &
  if [[ 0 -eq $(($i % 4)) ]]  ; then wait ; fi
done
wait
rm -f cf3.[0-9]*.result &
sort -mnr -T $CF3DIR/tmp/ s.*result > cf3.sorted.result
rm -f s.*.result &
./filter_300.pl cf3.sorted.result > cf3.result

rm -f cf3.sorted.result

rsync -a $CF3DIR/cf3.result $CFDB:$CF3DIR/tmp/cf3.result
ssh $CFDB $CF3DIR/mysql_load_user

