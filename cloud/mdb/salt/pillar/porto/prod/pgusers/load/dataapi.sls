data:
    config:
        pgusers:
            dataapi:
                password: {{ salt.yav.get('ver-01e11h9dzxsajvf4td9gtbtp6q[password]') }}
                allow_port: '6432'
                allow_db: api_disk_data
                superuser: True # temporary for dbrylev@
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - api_disk_data
                settings:
                    log_statement: 'mod'

