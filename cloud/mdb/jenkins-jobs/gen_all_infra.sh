#!/bin/sh

./gen_arcadia_infra_job.sh clickhouse test_clickhouse
./gen_arcadia_infra_job.sh clickhouse_backup_service test_clickhouse_backup_service
./gen_arcadia_infra_job.sh clickhouse_go_api test_clickhouse_go_api
./gen_arcadia_infra_job.sh clickhouse_keeper test_clickhouse_keeper
./gen_arcadia_infra_job.sh clickhouse_scale_upgrade test_clickhouse_scale_upgrade
./gen_arcadia_infra_job.sh clickhouse_sql_management test_clickhouse_sql_management
./gen_arcadia_infra_job.sh clickhouse_single test_clickhouse_single
./gen_arcadia_infra_job.sh clickhouse_single_scale_upgrade test_clickhouse_single_scale_upgrade
./gen_arcadia_infra_job.sh clickhouse_single_sql_management test_clickhouse_single_sql_management
./gen_arcadia_infra_job.sh clickhouse_single_cloud_storage test_clickhouse_single_cloud_storage
./gen_arcadia_infra_job.sh clickhouse_single_to_ha test_clickhouse_single_to_ha
./gen_arcadia_infra_job.sh clickhouse_tls test_clickhouse_tls
./gen_arcadia_infra_job.sh common test
./gen_arcadia_infra_job.sh mongodb_4_0 test_mongodb_4_0
./gen_arcadia_infra_job.sh mongodb_4_0_single test_mongodb_4_0_single
./gen_arcadia_infra_job.sh mongodb_4_0_sharded test_mongodb_4_0_sharded
./gen_arcadia_infra_job.sh mongodb_4_0_sharded_part2 test_mongodb_4_0_sharded_part2
./gen_arcadia_infra_job.sh mongodb_4_2 test_mongodb_4_2
./gen_arcadia_infra_job.sh mongodb_4_2_single test_mongodb_4_2_single
./gen_arcadia_infra_job.sh mongodb_4_2_sharded test_mongodb_4_2_sharded
./gen_arcadia_infra_job.sh mongodb_4_2_sharded_part2 test_mongodb_4_2_sharded_part2
./gen_arcadia_infra_job.sh mongodb_4_4 test_mongodb_4_4
./gen_arcadia_infra_job.sh mongodb_4_4_single test_mongodb_4_4_single
./gen_arcadia_infra_job.sh mongodb_4_4_sharded test_mongodb_4_4_sharded
./gen_arcadia_infra_job.sh mongodb_4_4_sharded_part2 test_mongodb_4_4_sharded_part2
./gen_arcadia_infra_job.sh mongodb_4_4_enterprise test_mongodb_4_4_enterprise
./gen_arcadia_infra_job.sh mongodb_5_0 test_mongodb_5_0
./gen_arcadia_infra_job.sh mongodb_5_0_single test_mongodb_5_0_single
./gen_arcadia_infra_job.sh mongodb_5_0_sharded test_mongodb_5_0_sharded
./gen_arcadia_infra_job.sh mongodb_5_0_sharded_part2 test_mongodb_5_0_sharded_part2
./gen_arcadia_infra_job.sh mongodb_5_0_enterprise test_mongodb_5_0_enterprise
./gen_arcadia_infra_job.sh mongodb_upgrade_4_2 test_mongodb_upgrade_4_2
./gen_arcadia_infra_job.sh mongodb_upgrade_4_4 test_mongodb_upgrade_4_4
./gen_arcadia_infra_job.sh mongodb_upgrade_5_0 test_mongodb_upgrade_5_0
./gen_arcadia_infra_job.sh mongodb_sharded_upgrade_4_2 test_mongodb_sharded_upgrade_4_2
./gen_arcadia_infra_job.sh mongodb_sharded_upgrade_4_4 test_mongodb_sharded_upgrade_4_4
./gen_arcadia_infra_job.sh mongodb_sharded_upgrade_5_0 test_mongodb_sharded_upgrade_5_0
./gen_arcadia_infra_job.sh mysql test_mysql
./gen_arcadia_infra_job.sh mysql_8_0 test_mysql_8_0
./gen_arcadia_infra_job.sh mysql_upgrade_80 test_mysql_upgrade_80
./gen_arcadia_infra_job.sh mysql_single test_mysql_single
./gen_arcadia_infra_job.sh postgresql_single test_postgresql_single
./gen_arcadia_infra_job.sh postgresql test_postgresql
./gen_arcadia_infra_job.sh postgresql_1c test_postgresql_1c
./gen_arcadia_infra_job.sh postgresql_11 test_postgresql_11
./gen_arcadia_infra_job.sh postgresql_11_1c test_postgresql_11_1c
./gen_arcadia_infra_job.sh postgresql_12 test_postgresql_12
./gen_arcadia_infra_job.sh postgresql_12_1c test_postgresql_12_1c
./gen_arcadia_infra_job.sh postgresql_13 test_postgresql_13
./gen_arcadia_infra_job.sh postgresql_13_1c test_postgresql_13_1c
./gen_arcadia_infra_job.sh postgresql_14 test_postgresql_14
./gen_arcadia_infra_job.sh postgresql_14_1c test_postgresql_14_1c
./gen_arcadia_infra_job.sh postgresql_upgrade_11 test_postgresql_upgrade_11
./gen_arcadia_infra_job.sh postgresql_upgrade_12 test_postgresql_upgrade_12
./gen_arcadia_infra_job.sh postgresql_upgrade_13 test_postgresql_upgrade_13
./gen_arcadia_infra_job.sh postgresql_upgrade_14 test_postgresql_upgrade_14
./gen_arcadia_infra_job.sh postgresql_upgrade_all_versions test_postgresql_upgrade_all_versions
./gen_arcadia_infra_job.sh postgresql_upgrade_all_versions_1c test_postgresql_upgrade_all_versions_1c
./gen_arcadia_infra_job.sh redis test_redis
./gen_arcadia_infra_job.sh redis_single test_redis_single
./gen_arcadia_infra_job.sh redis_sharded test_redis_sharded
./gen_arcadia_infra_job.sh redis_6 test_redis_6
./gen_arcadia_infra_job.sh redis_62_single test_redis_62_single
./gen_arcadia_infra_job.sh redis_6_sharded test_redis_6_sharded
./gen_arcadia_infra_job.sh redis_62_tls test_redis_62_tls
./gen_arcadia_infra_job.sh redis_62_tls_sharded test_redis_62_tls_sharded
./gen_arcadia_infra_job.sh redis_7_cluster_tls test_redis_7_cluster_tls
./gen_arcadia_infra_job.sh redis_7_sharded_tls test_redis_7_sharded_tls
./gen_arcadia_infra_job.sh opensearch_single test_opensearch_single
./gen_arcadia_infra_job.sh opensearch_cluster test_opensearch_cluster
./gen_arcadia_infra_job.sh elasticsearch_7_10 test_elasticsearch_7_10
./gen_arcadia_infra_job.sh elasticsearch_7_17 test_elasticsearch_7_17
./gen_arcadia_infra_job.sh elasticsearch_single_7_10 test_elasticsearch_single_7_10
./gen_arcadia_infra_job.sh elasticsearch_single_7_11 test_elasticsearch_single_7_11
./gen_arcadia_infra_job.sh elasticsearch_single_7_12 test_elasticsearch_single_7_12
./gen_arcadia_infra_job.sh elasticsearch_single_7_13 test_elasticsearch_single_7_13
./gen_arcadia_infra_job.sh elasticsearch_single_7_14 test_elasticsearch_single_7_14
./gen_arcadia_infra_job.sh elasticsearch_single_7_15 test_elasticsearch_single_7_15
./gen_arcadia_infra_job.sh elasticsearch_single_7_16 test_elasticsearch_single_7_16
./gen_arcadia_infra_job.sh elasticsearch_single_7_17 test_elasticsearch_single_7_17
./gen_arcadia_infra_job.sh elasticsearch_scale_upgrade test_elasticsearch_scale_upgrade
./gen_arcadia_infra_job.sh greenplum test_greenplum
./gen_arcadia_infra_job.sh deploy_v2 test_deploy_v2

./gen_arcadia_dataproc_infra_job.sh start_sqlserver_env sqlserver_enterprise_multi
./gen_arcadia_dataproc_infra_job.sh start_sqlserver_env sqlserver_enterprise_single
./gen_arcadia_dataproc_infra_job.sh start_sqlserver_env sqlserver_standard_multi
./gen_arcadia_dataproc_infra_job.sh start_sqlserver_env sqlserver_standard_single

ARCADIA_DOWNSTREAM=$(ls arcadia_infrastructure*.groovy | sort | sed 's/\.groovy//g' | xargs echo | sed 's/\ /\,/g')

for file in $(grep %ARCADIA_DOWNSTREAM_PROJECTS% ./*.groovy | cut -d: -f1)
do
    sed -i "s/%ARCADIA_DOWNSTREAM_PROJECTS%/$ARCADIA_DOWNSTREAM/g" "${file}"
done
