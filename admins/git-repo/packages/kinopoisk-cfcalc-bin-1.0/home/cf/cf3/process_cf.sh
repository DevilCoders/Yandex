#!/usr/bin/env bash

CF3DIR=/home/cf/cf3
mkdir -p /home/cf/cf3/tmp
CFDB=cfdb-dbs.kp.yandex.net

while ! mkdir /tmp/cf.lock &>/dev/null ; do sleep 0.5 ; done
trap 'rm -r /tmp/cf.lock' EXIT

cd $CF3DIR
BACKENDS=`curl -s http://c.yandex-team.ru/api-cached/groups2hosts/kp-backend`
for i in $BACKENDS; do ssh -q -o "BatchMode=yes" -o "ConnectTimeout=1" root@$i "echo 2>&1" && echo $i OK || echo $i NO; done | grep OK | awk '{print $1}' | sed -E "s/(.*)/\1:5/" | uniq > $CF3DIR/hosts
BACKALL=`cat $CF3DIR/hosts`
CFCALC=`curl -s http://c.yandex-team.ru/api-cached/groups2hosts/kp-cfcalc |sed -E "s/(.*)/\1:7/"`
HOSTS=$BACKALL" "$CFCALC

N=0

mysql -h $CFDB -N cf2 -e 'SELECT v.user_id,v.id, v.vote FROM kinopoisk.users_rel_vote v INNER JOIN kinopoisk.user_count_vote u USING (user_id) where v.vote>0 AND u.count_vote >= 20 ORDER BY v.user_id,v.id;' > urv.csv
# for per-user recalc
./convert_for_sorting < urv.csv > urv.fix.csv

USERS=`cat urv.csv | ./prepare_cf3`

for h in $HOSTS ; do
    N=$(($N + `echo $h |cut -d: -f 2 ` ))
    HOST=`echo $h |cut -d: -f 1 `
    rsync -a --exclude '*.result' --exclude '*.csv' $CF3DIR/ $HOST:$CF3DIR/
done
wait
CUR=1
for h in $HOSTS ; do
    HOST=`echo $h |cut -d: -f 1 `
    HN=`echo $h |cut -d: -f 2 `
    for i in `seq 1 $HN` ; do
        ssh $HOST $CF3DIR/cf3.sh -u $USERS -p $N -c $CUR > cf3-all.$CUR.result &
        CUR=$(( $CUR +1 ))
    done
done
wait
for i in `seq 1 $N` ; do
  sort -nr -T $CF3DIR/tmp/ cf3-all.$i.result > s.cf3-all.$i.result &
  if [[ 0 -eq $(($i % 4)) ]]  ; then wait ; fi
done
wait
rm -f cf3-all.[0-9]*.result &
sort -mnr -T $CF3DIR/tmp/ s.*result > cf3-all.sorted.result
rm -f s.*.result &
./filter_300.pl cf3-all.sorted.result > cf3-all.result
rm -f cf3-all.sorted.result

rsync -a $CF3DIR/cf3-all.result $CFDB:$CF3DIR/tmp/cf3-all.result
mysql -h $CFDB -f cf2 -e 'DROP TABLE IF EXISTS pre_cf_groups;create table pre_cf_groups(`user_id` int(5) NOT NULL, `rel_user_id` int(5) NOT NULL, `num_up` int(11) NOT NULL, `num_all` int(11) NOT NULL, `proximity` float(6,2) unsigned NOT NULL, PRIMARY KEY (`user_id`,`rel_user_id`), UNIQUE KEY `rel_user_id_uni` (`rel_user_id`,`user_id`), KEY `rel_user_id` (`rel_user_id`), KEY `num_all` (`num_all`), KEY `proximity` (`proximity`) ) ENGINE=MyISAM DEFAULT CHARSET=cp1251;'
ssh $CFDB $CF3DIR/mysql_load
mysql -h $CFDB -f cf2_new -e 'DROP TABLE IF EXISTS cf_groups;create table cf_groups(`user_id` int(5) NOT NULL, `rel_user_id` int(5) NOT NULL, `num_up` int(11) NOT NULL, `num_all` int(11) NOT NULL, `proximity` float(6,2) unsigned NOT NULL, PRIMARY KEY (`user_id`,`rel_user_id`), UNIQUE KEY `rel_user_id_uni` (`rel_user_id`,`user_id`), KEY `rel_user_id` (`rel_user_id`), KEY `num_all` (`num_all`), KEY `proximity` (`proximity`) ) ENGINE=InnoDB DEFAULT CHARSET=cp1251;'
ssh $CFDB rm $CF3DIR/tmp/cf3-all.result
