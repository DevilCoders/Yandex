data:
    config:
        pgusers:
            cms:
                password: {{ salt.yav.get('ver-01f2h24pmtjpz4meeq8x7gans7[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - cmsdb
                    - dbaas_metadb
