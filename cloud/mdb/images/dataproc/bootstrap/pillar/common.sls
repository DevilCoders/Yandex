data:
    services: [hdfs, yarn, mapreduce, tez, hive, hbase, zookeeper, sqoop, zeppelin, spark, flume, oozie, livy]
    config:
        hadoop_config_path: '/etc/hadoop/conf'
        hive_config_path: '/etc/hive/conf'
    settings:
        hive_driver: 'org.postgresql.Driver'
        hive_db_name: metastore
        hive_db_user: hive
        hive_db_password: 'hive-p2ssw0rd'
        truststore_password: 'changeit'
        oozie:
            driver: 'org.postgresql.Driver'
            db_name: oozie-db
            db_user: oozie
            db_password: 'oozie-p2ssw0rd'
    properties:
        hdfs:
            dfs.replication: 1
    s3:
        endpoint_url: 'storage.yandexcloud.net'
        region_name: 'ru-central1'
