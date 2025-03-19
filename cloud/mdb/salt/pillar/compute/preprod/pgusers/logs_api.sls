data:
  config:
    pgusers:
      logs_api:
        password: {{ salt.yav.get('ver-01fqksq20wtq0400ded806n845[password]') }}
        allow_port: '6432'
        allow_db: '*'
        superuser: False
        replication: False
        login: False
        create: True
        bouncer: True
        conn_limit: 5
        connect_dbs:
            - dbaas_metadb
