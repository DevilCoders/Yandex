data:
  dbaas_metadb:
    role_pillars:
      - type: clickhouse_cluster
        role: clickhouse_cluster
        value:
          data:
            mdb_metrics:
              enable_userfault_broken_collector: false
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_clickhouse_getter.py
                yasm_tags_db: clickhouse
            cloud_storage:
              s3:
                scheme: https
                endpoint: s3.yandexcloud.net
                virtual_hosted_style: true
      - type: clickhouse_cluster
        role: zk
        value:
          data:
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_zk_getter.py
                yasm_tags_db: zookeeper
      - type: kafka_cluster
        role: kafka_cluster
        value:
          data:
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/yasmagent/kafka_getter.py
      - type: kafka_cluster
        role: zk
        value:
          data:
            mdb_metrics:
              main:
                yasm_tags_cmd: /usr/local/yasmagent/mdb_zk_getter.py
                yasm_tags_db: zookeeper
