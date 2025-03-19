set-master-pillar:
    postgresql_cmd.set_master:
        - user: admin
        - port: 5432
        - password: {{ salt['pillar.get']('data:config:pgusers:admin:password') }}
        - timeout: {{ salt['pillar.get']('pg-master-timeout', 600) }}
