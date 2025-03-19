data:
    config:
        pgusers:
            deploy_api:
                password: {{ salt.yav.get('ver-01dz63dpeqvax928rt8jj9b70c[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb
