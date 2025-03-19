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
            clickhouse:
              tcp_port: 9001
            cloud_storage:
              s3:
                scheme: http
                endpoint: s3.mds.yandex.net
                virtual_hosted_style: true
                access_key_id:
                  data: {{ salt.yav.get('ver-01dzttw74se00x4jh81jjgwskq[id]') }}
                  encryption_version: 1
                access_secret_key:
                  data: {{ salt.yav.get('ver-01dzttw74se00x4jh81jjgwskq[secret]') }}
                  encryption_version: 1
                proxy_resolver:
                  uri: http://s3.mds.yandex.net/hostname
                  proxy_port:
                    http:
                      4080
                    https:
                      4443
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
