data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3api]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 1000
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_ro:
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3api_ro]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 2000
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_list:
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3api_list]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 200
                connect_dbs:
                    - s3db
                    - s3meta

            s3cleanup:
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3cleanup]') }}
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
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3util]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 40
                connect_dbs:
                    - 's3db'
                    - 's3meta'
                settings:
                    lock_timeout: '60s'
                    temp_file_limit: -1
                pgpass: True
                port: 6432

            s3service:
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3service]') }}
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
                password: {{ salt.yav.get('ver-01fp062bmyfr0q2y549mmth4md[s3maintain]') }}
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
            s3monitor:
                password: {{ salt.yav.get('ver-01g514xgzcrsthv9e8cg5bjm5c[password]') }}
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
            s3devops:
                password: {{ salt.yav.get('ver-01g514xyc22h03ywxzkvez9jnj[password]') }}
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
