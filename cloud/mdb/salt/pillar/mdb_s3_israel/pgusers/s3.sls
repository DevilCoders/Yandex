data:
    config:
        pgusers:
            s3api:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 1000
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcn87op4lj073ii3mjn7").password }}'
                replication: false
                superuser: false
            s3api_list:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 200
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcnm7tsolruhr1ifcgmo").password }}'
                replication: false
                superuser: false
            s3api_ro:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 2000
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcn1l2lkgbjevergc0lc").password }}'
                replication: false
                superuser: false
            s3cleanup:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 5
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcnchtiq9de059jm3l8t").password }}'
                replication: false
                superuser: false
            s3maintain:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 20
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcn8pak05uknqd9tgd5h").password }}'
                replication: false
                superuser: false
            s3service:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 3
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcnh1asmvftjdj9r57tl").password }}'
                replication: false
                superuser: false
            s3util:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 20
                connect_dbs:
                - s3db
                - s3meta
                create: true
                password: '{{ salt.lockbox.get("bcn2c7qarkic5mjpp2r7").password }}'
                pgpass: true
                port: 6432
                replication: false
                settings:
                    lock_timeout: 60s
                    temp_file_limit: -1
                superuser: false
