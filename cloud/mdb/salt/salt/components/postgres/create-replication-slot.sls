{% from "components/postgres/pg.jinja" import pg with context %}

create_replication_slot:
    postgresql_cmd.create_replication_slot:
        - name: {{ salt['grains.get']('fqdn')|replace('.', '_')|replace('-', '_') }}
        - user: admin
        - port: 5432
        - password: {{ salt['pillar.get']('data:config:pgusers:admin:password') }}
        - control_path: {{ pg.data }}/global/pg_control
        - force: {{ salt['pillar.get']('service-restart', False) }}
