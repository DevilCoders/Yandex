data:
    config:
        pgusers:
            secrets_api:
                password: {{ salt.yav.get('ver-01dvz3ttrbcphd33y41gr9q3kq[password]') }}
                allow_port: '6432'
                allow_db: secretsdb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - secretsdb
