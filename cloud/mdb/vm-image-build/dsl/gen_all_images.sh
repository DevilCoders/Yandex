#!/bin/sh

./gen_image_job.sh compute_common_1if common-1if-upload
./gen_image_job.sh compute_common_2if common-2if-upload
./gen_image_job.sh compute_clickhouse clickhouse-upload
./gen_image_job.sh compute_zookeeper zookeeper-upload
./gen_image_job.sh compute_mongodb mongodb-upload
./gen_image_job.sh compute_redis redis-upload
./gen_image_job.sh compute_mysql mysql-upload
while read -r ver; do
    ./gen_image_job.sh compute_sqlserver_${ver} sqlserver-${ver}-upload
done < ../versions/sqlserver
./gen_image_job.sh compute_windows-witness windows-witness-upload
./gen_image_job.sh compute_wsus_common wsus-common-upload
./gen_image_job.sh compute_kafka kafka-upload
./gen_image_job.sh compute_elasticsearch elasticsearch-upload
./gen_image_job.sh porto_postgresql postgresql-porto-upload
./gen_image_job.sh porto_clickhouse clickhouse-porto-upload
./gen_image_job.sh porto_zookeeper zookeeper-porto-upload
./gen_image_job.sh porto_mongodb mongodb-porto-upload
./gen_image_job.sh porto_redis redis-porto-upload
while read -r ver; do
    ./gen_image_job.sh porto_redis_${ver} redis-${ver}-porto-upload
    ./gen_image_job.sh compute_redis_${ver} redis-${ver}-upload
done < ../versions/redis
./gen_image_job.sh porto_mysql mysql-porto-upload
./gen_image_job.sh porto_kafka kafka-porto-upload
./gen_image_job.sh porto_elasticsearch elasticsearch-porto-upload
./gen_image_job.sh compute_opensearch opensearch-upload
./gen_image_job.sh porto_opensearch opensearch-porto-upload
while read -r ver; do
    ./gen_image_job.sh porto_opensearch_${ver} opensearch-${ver}-porto-upload
    ./gen_image_job.sh compute_opensearch_${ver} opensearch-${ver}-upload
done < ../versions/opensearch
./gen_image_job.sh compute_postgresql postgresql-upload
while read -r ver; do
    ./gen_image_job.sh porto_postgresql_${ver} postgresql-${ver}-porto-upload
    ./gen_image_job.sh compute_postgresql_${ver} postgresql-${ver}-compute-upload
done < ../versions/pg
./gen_image_job.sh porto_greenplum greenplum-porto-upload
./gen_image_job.sh compute_greenplum greenplum-upload
