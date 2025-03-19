data:
    config:
        pgusers:
            mdb_report:
                password: {{ salt.yav.get('ver-01e98cs45p5d2cw505s3mdrffq[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb
