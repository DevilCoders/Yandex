data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01e1c37heryq1y98y3br71xtwb[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 500
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_ro:
                password: {{ salt.yav.get('ver-01e1c56gyyn0bez7rkcqqm2s2z[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 3000
                connect_dbs:
                    - s3db
                    - s3meta

            s3api_list:
                password: {{ salt.yav.get('ver-01e1c57438jkztfh02ryf4r4zh[password]') }}
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
                password: {{ salt.yav.get('ver-01e1c57hvezamnvb1nj6cd2a9n[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 7
                connect_dbs:
                    - 's3db'
                    - 's3meta'

            s3util:
                password: {{ salt.yav.get('ver-01e1c586w8dgz38qm50ax13fts[password]') }}
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
                password: {{ salt.yav.get('ver-01ec0a6an04cfs69fjpj872qnh[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 6
                connect_dbs:
                    - 's3db'
                    - 's3meta'

            s3maintain:
                password: {{ salt.yav.get('ver-01fp06hejdzh5gb7zgt7tgfc0v[s3maintain]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 8
                connect_dbs:
                    - 's3db'
                    - 's3meta'

            s3replication:
                password: {{ salt.yav.get('ver-01g280hx6rv28nwe59dpw7akp4[s3replication]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: True
                conn_limit: 50
                settings:
                    wal_sender_timeout: '12h'
                connect_dbs:
                    - s3db
            s3monitor:
                password: {{ salt.yav.get('ver-01g514jhrm7b8dd4z9p4bykgj6[password]') }}
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
                password: {{ salt.yav.get('ver-01g514hyrnhxwf1k7bb6jqf57f[password]') }}
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
