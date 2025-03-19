data:
  cms:
    enabled_mw:
      services:
        - greenplum_master
        - greenplum_segments
        - greenplum_segments_any_alive
        - greenplum_segments_any_primary
        - mongod
        - mongos
        - mongocfg
        - mysql
        - odyssey
        - pg_replication
        - pgbouncer
        - redis_cluster
        - redis
        - sentinel
      roles:
        - greenplum_cluster
        - greenplum_cluster.segment_subcluster
        - mongodb_cluster
        - mongodb_cluster.mongod
        - mysql_cluster
        - postgresql_cluster
        - redis_cluster
