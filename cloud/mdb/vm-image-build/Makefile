.PHONY: all clean postgresql-upload postgresql-preprod-upload \
	clickhouse-upload clickhouse-preprod-upload \
	zookeeper-upload zookeeper-preprod-upload \
	common-1if-upload common-1if-preprod-upload \
	common-2if-upload common-2if-preprod-upload \
	mongodb-upload mongodb-preprod-upload \
	mysql-upload mysql-preprod-upload \
	sqlserver-upload sqlserver-preprod-upload \
	redis-upload redis-preprod-upload \
	kafka-upload kafka-preprod-upload \
	elasticsearch-upload elasticsearch-preprod-upload \
	opensearch-upload opensearch-preprod-upload \
	greenplum-upload greenplum-preprod-upload wsus-common-upload \
	wsus-common-preprod-upload windows-witness-upload \
	windows-witness-preprod-upload

all: common-1if-upload common-2if-upload mysql-upload redis-upload zookeeper-upload \
	postgresql-upload clickhouse-upload mongodb-upload sqlserver-upload kafka-upload elasticsearch-upload \
	greenplum-upload wsus-common-upload windows-witness-upload opensearch-upload

base-bionic.img:
	qemu-img create -f qcow2 base-bionic.img 20G
	flock /tmp/qemu-nbd.lock sudo timeout 3600 ./base_bionic_setup.sh base-bionic.img
	sudo chmod 666 base-bionic*

clean:
	sudo rm -rf *.img *.log temp** python-common* *.img.gz

postgresql.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh postgresql pg base-bionic.img compute
postgresql-%.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh postgresql-$* pg-$* base-bionic.img compute

postgresql-%-preprod-upload: postgresql-%.img
	timeout 10800 ./upload_compute.sh postgresql-$*
postgresql-preprod-upload: postgresql.img
	timeout 10800 ./upload_compute.sh postgresql

postgresql-%-compute-upload: postgresql-%-preprod-upload
	sudo truncate -s 0 postgresql-$*.img

postgresql-upload: postgresql-preprod-upload
	sudo truncate -s 0 postgresql.img

clickhouse.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh clickhouse ch base-bionic.img compute

clickhouse-preprod-upload: clickhouse.img
	timeout 10800 ./upload_compute.sh clickhouse

clickhouse-upload: clickhouse-preprod-upload
	sudo truncate -s 0 clickhouse.img

mongodb.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh mongodb mongodb base-bionic.img compute

mongodb-preprod-upload: mongodb.img
	timeout 10800 ./upload_compute.sh mongodb

mongodb-upload: mongodb-preprod-upload
	sudo truncate -s 0 mongodb.img

mysql.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh mysql mysql base-bionic.img compute

mysql-preprod-upload: mysql.img
	timeout 10800 ./upload_compute.sh mysql

mysql-upload: mysql-preprod-upload
	sudo truncate -s 0 mysql.img

sqlserver.img:
	flock /tmp/qemu.lock bash -c 'cd ./windows-common && sudo timeout 7200 ./build.sh sqlserver'
sqlserver-%.img:
	flock /tmp/qemu.lock bash -c 'cd ./windows-common && sudo timeout 7200 ./build.sh sqlserver $*'

sqlserver-preprod-upload: sqlserver.img
	timeout 10800 ./upload_compute.sh sqlserver
sqlserver-%-preprod-upload: sqlserver-%.img
	timeout 10800 ./upload_compute.sh sqlserver-$*

sqlserver-upload: sqlserver-preprod-upload
	sudo truncate -s 0 sqlserver.img
sqlserver-%-upload: sqlserver-%-preprod-upload
	sudo truncate -s 0 sqlserver-$*.img

wsus-common.img:
	flock /tmp/qemu.lock bash -c 'cd ./windows-common && sudo timeout 7200 ./build.sh wsus common'

wsus-common-preprod-upload: wsus-common.img
	timeout 10800 ./upload_compute.sh wsus-common

wsus-common-upload: wsus-common-preprod-upload
	sudo truncate -s 0 wsus-common.img

windows-witness.img:
	flock /tmp/qemu.lock bash -c 'cd ./windows-common && sudo timeout 7200 ./build.sh windows-witness'

windows-witness-preprod-upload: windows-witness.img
	timeout 10800 ./upload_compute.sh windows-witness

windows-witness-upload: windows-witness-preprod-upload
	sudo truncate -s 0 windows-witness.img

zookeeper.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh zookeeper zk base-bionic.img compute

zookeeper-preprod-upload: zookeeper.img
	timeout 10800 ./upload_compute.sh zookeeper

zookeeper-upload: zookeeper-preprod-upload
	sudo truncate -s 0 zookeeper.img

redis.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh redis redis base-bionic.img compute

redis-%.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh redis-$* redis.$* base-bionic.img compute

redis-preprod-upload: redis.img
	timeout 10800 ./upload_compute.sh redis

redis-%-preprod-upload: redis-%.img
	timeout 10800 ./upload_compute.sh redis-$*

redis-upload: redis-preprod-upload
	sudo truncate -s 0 redis.img

redis-%-upload: redis-%-preprod-upload
	sudo truncate -s 0 redis-$*.img

kafka.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh kafka kafka base-bionic.img compute

kafka-preprod-upload: kafka.img
	timeout 10800 ./upload_compute.sh kafka

kafka-upload: kafka-preprod-upload
	sudo truncate -s 0 kafka.img

elasticsearch.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh elasticsearch elasticsearch base-bionic.img compute

elasticsearch-preprod-upload: elasticsearch.img
	timeout 10800 ./upload_compute.sh elasticsearch

elasticsearch-upload: elasticsearch-preprod-upload
	sudo truncate -s 0 elasticsearch.img

elasticsearch-%.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh elasticsearch-$* elasticsearch-$* base-bionic.img compute

elasticsearch-%-preprod-upload: elasticsearch-%.img
	timeout 10800 ./upload_compute.sh elasticsearch-$*

elasticsearch-%-upload: elasticsearch-%-preprod-upload
	sudo truncate -s 0 elasticsearch-$*.img

opensearch.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh opensearch opensearch base-bionic.img compute

opensearch-preprod-upload: opensearch.img
	timeout 10800 ./upload_compute.sh opensearch

opensearch-upload: opensearch-preprod-upload
	sudo truncate -s 0 opensearch.img

opensearch-%.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh opensearch-$* opensearch.$* base-bionic.img compute

opensearch-%-preprod-upload: opensearch-%.img
	timeout 10800 ./upload_compute.sh opensearch-$*

opensearch-%-upload: opensearch-%-preprod-upload
	sudo truncate -s 0 opensearch-$*.img

common-1if.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh common-1if compute_controlplane base-bionic.img compute

common-1if-preprod-upload: common-1if.img
	timeout 10800 ./upload_compute.sh common-1if

common-1if-upload: common-1if-preprod-upload
	sudo truncate -s 0 common-1if.img

common-2if.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh common-2if compute_e2e base-bionic.img compute

common-2if-preprod-upload: common-2if.img
	timeout 10800 ./upload_compute.sh common-2if

common-2if-upload: common-2if-preprod-upload
	sudo truncate -s 0 common-2if.img

postgresql_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh postgresql pg base-bionic.img porto

postgresql_%_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh postgresql-$* pg-$* base-bionic.img porto

clickhouse_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh clickhouse ch base-bionic.img porto

mongodb_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh mongodb mongodb base-bionic.img porto

mysql_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh mysql mysql base-bionic.img porto

redis_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh redis redis base-bionic.img porto

redis_%_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh redis-$* redis.$* base-bionic.img porto

zookeeper_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh zookeeper zk base-bionic.img porto

kafka_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh kafka kafka base-bionic.img porto

postgresql-porto-upload: postgresql_porto.img.gz
	timeout 10800 ./upload_porto.sh postgresql_porto.img.gz postgresql-bionic
postgresql-%-porto-upload: postgresql_%_porto.img.gz
	timeout 10800 ./upload_porto.sh postgresql-$*_porto.img.gz postgresql-$*-bionic

clickhouse-porto-upload: clickhouse_porto.img.gz
	timeout 10800 ./upload_porto.sh clickhouse_porto.img.gz clickhouse-bionic

mongodb-porto-upload: mongodb_porto.img.gz
	timeout 10800 ./upload_porto.sh mongodb_porto.img.gz mongodb-bionic

mysql-porto-upload: mysql_porto.img.gz
	timeout 10800 ./upload_porto.sh mysql_porto.img.gz mysql-bionic

redis-porto-upload: redis_porto.img.gz
	timeout 10800 ./upload_porto.sh redis_porto.img.gz redis-bionic

redis-%-porto-upload: redis_%_porto.img.gz
	timeout 10800 ./upload_porto.sh redis-$*_porto.img.gz redis-$*-bionic

zookeeper-porto-upload: zookeeper_porto.img.gz
	timeout 10800 ./upload_porto.sh zookeeper_porto.img.gz zookeeper-bionic

kafka-porto-upload: kafka_porto.img.gz
	timeout 10800 ./upload_porto.sh kafka_porto.img.gz kafka-bionic

elasticsearch_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh elasticsearch elasticsearch base-bionic.img porto

elasticsearch-porto-upload: elasticsearch_porto.img.gz
	timeout 10800 ./upload_porto.sh elasticsearch_porto.img.gz elasticsearch-bionic

elasticsearch_%_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh elasticsearch-$* elasticsearch-$* base-bionic.img porto

elasticsearch-%-porto-upload: elasticsearch_%_porto.img.gz
	timeout 10800 ./upload_porto.sh elasticsearch-$*_porto.img.gz elasticsearch-$*-bionic

opensearch_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh opensearch opensearch base-bionic.img porto

opensearch-porto-upload: opensearch_porto.img.gz
	timeout 10800 ./upload_porto.sh opensearch_porto.img.gz opensearch-bionic

opensearch_%_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh opensearch-$* opensearch.$* base-bionic.img porto

opensearch-%-porto-upload: opensearch_%_porto.img.gz
	timeout 10800 ./upload_porto.sh opensearch-$*_porto.img.gz opensearch-$*-bionic

greenplum_porto.img.gz: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh greenplum gp base-bionic.img porto

greenplum-porto-upload: greenplum_porto.img.gz
	timeout 10800 ./upload_porto.sh greenplum_porto.img.gz greenplum-bionic

greenplum.img: base-bionic.img
	flock /tmp/qemu.lock sudo timeout 3600 ./db_setup.sh greenplum gp base-bionic.img compute

greenplum-preprod-upload: greenplum.img
	timeout 10800 ./upload_compute.sh greenplum

greenplum-upload: greenplum-preprod-upload
	sudo truncate -s 0 greenplum.img
