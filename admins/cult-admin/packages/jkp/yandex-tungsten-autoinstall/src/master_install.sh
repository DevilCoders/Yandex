DATABASE_USER="root"
DATABASE_PASSWORD=$(cat /etc/mysql/client.cnf | grep -A1 root | grep password | awk '{print $3}')
DATABASE_PORT=3306
if [ -d /opt/replicator ]; then action="update";
else action="install"; fi
./tools/tpm $action  mongodb \
  --reset \
  --master=127.0.0.1 \
  --log=timestamp \
  --replication-user=$DATABASE_USER \
  --replication-password=$DATABASE_PASSWORD \
  --replication-port=$DATABASE_PORT \
  --topology=master-slave \
  --datasource-type=mysql \
  --datasource-mysql-conf=/etc/mysql/my.cnf  \
  --datasource-log-directory=/var/log/mysql/ \
  --datasource-port=3306  \
  --home-directory=/opt/replicator \
  --thl-port=2112 \
  --java-file-encoding=UTF8 \
  --java-mem-size=4096 \
  --skip-validation-check=WriteableHomeDirectoryCheck \
  --skip-validation-check=MySQLNoMySQLReplicationCheck \
  --skip-validation-check=MySQLSettingsCheck \
  --skip-validation-check=HostsFileCheck \
  --skip-validation-check=MySQLMyISAMCheck \
  --mysql-use-bytes-for-string=false \
  --mysql-enable-enumtostring=true \
  --mysql-enable-settostring=true \
  --svc-extractor-filters=colnames,pkey \
  --property=replicator.filter.pkey.addPkeyToInserts=true \
  --property=replicator.filter.pkey.addColumnsToDeletes=true \
  --svc-parallelization-type=none --start-and-report
