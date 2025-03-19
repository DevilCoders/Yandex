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
        dataproc:
            cores_per_map_task: 1
            cores_per_reduce_task: 1
            cores_per_app_master: 1
            hdfs_namenode_memory_fraction: 0.4
            hadoop_client_memory_fraction: 0.25
            yarn_resourcemanager_memory_fraction: 0.4
            yarn_client_memory_fraction: 0.25
            yarn_scheduler_min_memory: 256
            spark_executor_memory_fraction: 0.3
            spark_executors_per_vm: 2
            spark_driver_memory_fraction: 0.25
            nodemanager_available_memory_ratio: 0.8
    s3:
        endpoint_url: 'storage.yandexcloud.net'
        region_name: 'ru-central1'
