data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3api]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 50
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_ro:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3api_ro]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 300
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_list:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3api_list]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 20
                connect_dbs:
                    - s3db
                    - s3meta

            s3cleanup:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3cleanup]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - 's3db'
                    - 's3meta'

            s3util:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3util]') }}
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
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3service]') }}
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

            s3maintain:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3maintain]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 20
                connect_dbs:
                    - 's3db'
                    - 's3meta'
