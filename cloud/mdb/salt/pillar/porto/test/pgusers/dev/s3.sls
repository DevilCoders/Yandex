data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01e983wa6zvt2xmq7g2t177hc1[password]') }}
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
                password: {{ salt.yav.get('ver-01e983zbyea3pgy03zdeqa3wct[password]') }}
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
                password: {{ salt.yav.get('ver-01e9843e9jzb66na7d04a3ma5n[password]') }}
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
                password: {{ salt.yav.get('ver-01e9847rfes10m4509cs1s3m8d[password]') }}
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
                password: {{ salt.yav.get('ver-01e9849tmbg6f8tj89dpxaxf2r[password]') }}
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
                password: {{ salt.yav.get('ver-01ec0afp0t0we726cpd0njr231[password]') }}
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
