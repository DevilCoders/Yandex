data:
    config:
        pgusers:
            secrets_api:
                password: {{ salt.yav.get('ver-01e07mp75ebx4wzwtzws2snkg0[password]') }}
                allow_port: '6432'
                allow_db: secretsdb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - secretsdb
