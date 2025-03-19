data:
    config:
        pgusers:
            pillar_secrets:
                password: {{ salt.yav.get('ver-01fqkkf9jtbvkd26wcj2fkqqsz[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                login: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - dbaas_metadb
