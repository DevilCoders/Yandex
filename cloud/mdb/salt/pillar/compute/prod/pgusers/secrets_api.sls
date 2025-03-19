data:
    config:
        pgusers:
            secrets_api:
                password: {{ salt.yav.get('ver-01e8a9pd6d5naecfj315kqyk59[password]') }}
                allow_port: '6432'
                allow_db: secretsdb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - secretsdb
