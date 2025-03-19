data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01e11hcwg5x0tyh5sh23tj73g8[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - s3meta
                    - s3db

            s3api_ro:
                password: {{ salt.yav.get('ver-01e11he4fzqqfvdx3anhheq9p7[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 600
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_list:
                password: {{ salt.yav.get('ver-01e11hf15q27w7cqaqqzqac004[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 40
                connect_dbs:
                    - s3db
                    - s3meta

            s3cleanup:
                password: {{ salt.yav.get('ver-01e11hg3txvy4cb7vkg0tqzaky[password]') }}
                allow_port: '6432'
                allow_db: s3db
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - 's3db'
                    - 's3meta'

            s3util:
                password: {{ salt.yav.get('ver-01e11hgtxq508brcyfn6yn4ms4[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 10
                connect_dbs:
                    - 's3db'
                    - 's3meta'
                settings:
                    lock_timeout: '60s'
                    temp_file_limit: -1
                pgpass: True
                port: 6432

            s3service:
                password: {{ salt.yav.get('ver-01ec0a3my2dw6spzfe7d116awv[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 3
                connect_dbs:
                    - 's3db'
                    - 's3meta'
