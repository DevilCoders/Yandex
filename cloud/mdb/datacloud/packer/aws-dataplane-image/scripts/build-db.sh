#!/bin/bash

set -x
set -e

if [ $# -lt 1 ]; then
  echo "usage: $0 <suite name>" 1>&2
  exit 1
fi

GRAINS="grains: {test: {suite: $1, vtype: aws}}"

cp /etc/salt/minion /etc/salt/minion.bak
echo $GRAINS >/etc/salt/minion
# Workaround for mongodb hosts (first highstate always fails)
salt-call saltutil.sync_all
salt-call state.highstate --output-diff || echo "First highstate failed"
salt-call state.highstate --output-diff 2>@1 | tee "$1_salt.log"
cat "$1_salt.log"
grep -qE 'Failed:\s+0' "$1_salt.log"
update-rc.d -f salt-minion remove
mv -f /etc/salt/minion.bak /etc/salt/minion
apt-get clean
truncate -s 0 /etc/hostname
mv -f /etc/hosts.bak /etc/hosts
rm -rf \
  /var/lib/postgresql/{1,2}* \
  /var/lib/postgresql/.pgpass \
  /home/mysql/.my.cnf \
  /home/mysql/.restore.my.cnf \
  /etc/postgresql/{1,2}* \
  /etc/postgresql/ssl/* \
  /etc/pgbouncer/ssl/* \
  /etc/pgbouncer/userlist.txt \
  /root/.pgpass \
  /root/.my.cnf \
  /var/lib/push-client* \
  /usr/local/yasmagent/CONF/agent.mailpostgresql.conf \
  /var/lib/clickhouse/* \
  /etc/clickhouse-server/* \
  /etc/cron.d/clickhouse-server* \
  /etc/systemd/system/multi-user.target.wants/clickhouse-server.service \
  /lib/systemd/system/clickhouse-server.service \
  /var/lib/mongodb/* \
  /etc/mongodb/* \
  /lib/systemd/system/mongodb.service \
  /etc/systemd/system/multi-user.target.wants/mongodb.service \
  /var/lib/zookeeper/* \
  /etc/zookeeper/* \
  /etc/systemd/system/multi-user.target.wants/zookeeper.service \
  /lib/systemd/system/zookeeper.service \
  /var/lib/elasticsearch/* \
  /etc/elasticsearch/* \
  /etc/systemd/system/multi-user.target.wants/elasticsearch.service \
  /lib/systemd/system/elasticsearch.service \
  /etc/systemd/system/multi-user.target.wants/kibana.service \
  /lib/systemd/system/kibana.service \
  /etc/kibana/* \
  /home/monitor/.pgpass \
  /home/monitor/.my.cnf \
  /var/cache/salt/minion/* \
  /etc/salt/minion_id \
  /etc/salt/pki/minion/minion.p* \
  /etc/yandex/mdb-deploy/deploy_version \
  /etc/yandex/mdb-deploy/mdb_deploy_api_host \
  /var/cache/apt/archives/*.deb \
  /var/log/journal/* \
  /var/lib/mysql/* \
  /etc/mysql/* \
  /etc/rc*/*mysql \
  /etc/systemd/system/multi-user.target.wants/mysql.service \
  /var/lib/redis/* \
  /etc/redis/* \
  /etc/systemd/system/multi-user.target.wants/redis-sentinel.service \
  /lib/systemd/system/redis-sentinel.service \
  /etc/systemd/system/multi-user.target.wants/redis-server.service \
  /lib/systemd/system/redis-server.service \
  /home/gpadmin/{.pgpass,.ssh,gpAdminLogs,gpconfigs} \
  /var/lib/greenplum/* \
  /etc/greenplum-pxf/*

find /var/log -name '*.gz' -delete
find /var/log -regextype posix-egrep -regex '.*[0-9]$' -delete
find /var/log -type f -exec truncate -s 0 {} \;

cloud-init clean
truncate -s 0 /home/ubuntu/.ssh/authorized_keys
truncate -s 0 /root/.ssh/authorized_keys

echo "SUCCESS!"
