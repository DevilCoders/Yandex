data:
    config:
        pgusers:
            smartcache_meta:
                password: {{ salt.yav.get('ver-01e11n7dat6qghv7tbs8jezwtj[password]') }}
                allow_port: '6432'
                allow_db: smartcache_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - smartcache_metadb
