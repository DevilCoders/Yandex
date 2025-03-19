data:
    config:
        pgusers:
            s3api:
                password: {{ salt.yav.get('ver-01dw731awwcfw57vg3wr863as2[password]') }}
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
                password: {{ salt.yav.get('ver-01dw732wdch7bsg5ga61v1n90q[password]') }}
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
                password: {{ salt.yav.get('ver-01dw733t9ph934gtemcxaxh5j2[password]') }}
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
                password: {{ salt.yav.get('ver-01dw735rac4h6kgbbneva2abh0[password]') }}
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
                password: {{ salt.yav.get('ver-01dw736vph85cf74hpw0b2x1k9[password]') }}
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
                password: {{ salt.yav.get('ver-01ec09na2scez1bedpjk7cavg4[password]') }}
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
                password: {{ salt.yav.get('ver-01fp06b3fndf2jnmyr51myfks5[s3maintain]') }}
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
            s3replication:
                password: {{ salt.yav.get('ver-01fb7e9h116qa38ytw689epx1z[password]') }}
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
                password: {{ salt.yav.get('ver-01g513pygk4ghdkncbm7vnr79v[password]') }}
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
                password: {{ salt.yav.get('ver-01g514g5ysnpr5bpg1kaghx5fy[password]') }}
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
