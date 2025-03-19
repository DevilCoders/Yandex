data:
    config:
        pgusers:
            dbaas_api:
                password: {{ salt.yav.get('ver-01e9dwm1s2c9p4fqd7b1d1x4nm[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb
