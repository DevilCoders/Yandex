data:
    config:
        pgusers:
            cms:
                password: {{ salt.yav.get('ver-01e57r9nz9fa5f0nsp5kbr6dhe[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - cmsdb
                    - dbaas_metadb
