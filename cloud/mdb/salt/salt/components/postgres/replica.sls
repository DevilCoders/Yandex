{% from "components/postgres/pg.jinja" import pg with context %}

postmaster.opts:
    file.absent:
        - name: /var/lib/postgresql/{{ pg.version.major }}/data/postmaster.opts
        - require_in:
            - service: postgresql-service
        - onlyif:
            - grep '/var/lib/pgsql' /var/lib/postgresql/{{ pg.version.major }}/data/postmaster.opts

pg-init:
    postgresql_cmd.replica_init:
        - name: {{ pg.data }}
{% if salt['pillar.get']('walg-restore', False) and salt['pillar.get']('data:use_walg', True) %}
        - method: wal-g
{% else %}
        - method: basebackup
        - version_major_num: {{ pg.version.major_num }}
{% endif %}
        - require:
            - file: {{ pg.data }}
            - file: {{ pg.prefix }}/.ssh/authorized_keys
            - file: {{ pg.prefix }}/.pgpass
            - cmd: locale-gen
{% if salt['pillar.get']('walg-restore', False) and salt['pillar.get']('data:use_walg', True) %}
            - pkg: walg-packages
            - file: /etc/wal-g/envdir
            - file: /etc/wal-g/wal-g.yaml
{% endif %}
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - file: {{ pg.data }}/conf.d/pg_hba.conf

include:
    - .service
    - .configs.recovery-conf
{% if salt['pillar.get']('data:dbaas:shard_hosts') %}
{%     if not salt['pillar.get']('pg-master', salt['pillar.get']('data:pgsync:replication_source')) %}
    - .set-master-pillar
{%     endif %}
{% endif %}
{% if salt['pillar.get']('data:use_replication_slots', True) %}
    - .create-replication-slot
{% endif %}

extend:
    postgresql-service:
        service.running:
            - require:
                - postgresql_cmd: recovery.conf
                - file: /var/log/postgresql
                - cmd: /etc/sysctl.d/postgres.conf
                - file: {{ pg.data }}/conf.d/postgresql.conf
                - file: {{ pg.data }}/conf.d/pg_hba.conf
{% if pg.version.major_num >= 1200 %}
                - file: {{ pg.data }}/standby.signal
{% endif %}
        cmd.wait:
            - require:
                - file: /usr/local/yandex/pg_wait_started.py

    recovery.conf:
        postgresql_cmd.populate_recovery_conf:
            - require:
                - file: /usr/local/yandex/populate_recovery_conf.py
{% if not salt['pillar.get']('service-restart', False) %}
                - postgresql_cmd: pg-init
{% else %}
{% if not salt['pillar.get']('pg-master', salt['pillar.get']('data:pgsync:replication_source')) %}
                - postgresql_cmd: set-master-pillar
{% endif %}
            - require_in:
                - cmd: postgresql-stop
{% endif %}

{% if salt['pillar.get']('data:use_replication_slots', True) %}
    create_replication_slot:
        postgresql_cmd.create_replication_slot:
        - require:
            - pkg: postgresql{{ pg.version.major }}-server
            - file: {{ pg.prefix }}/.pgpass
        - require_in:
            - postgresql_cmd: pg-init
{% endif %}

{% if salt['pillar.get']('data:dbaas:shard_hosts') %}
{%     if not salt['pillar.get']('pg-master', salt['pillar.get']('data:pgsync:replication_source')) %}
    set-master-pillar:
        postgresql_cmd.set_master:
            - require_in:
                - postgresql_cmd: pg-init
{%         if salt['pillar.get']('data:use_replication_slots', True) %}
                - postgresql_cmd: create_replication_slot
{%         endif %}
{%     endif %}
{% endif %}

postgresql-synced:
    cmd.run:
        - name: /usr/local/yandex/pg_wait_synced.py -w {{ salt.pillar.get('sync-timeout', salt.pillar.get('data:pgsync:recovery_timeout', '1200')) }}
        - runas: postgres
        - group: postgres
        - unless:
            - /usr/local/yandex/pg_wait_synced.py -w 3
        - require:
            - cmd: postgresql-service
            - file: /usr/local/yandex/pg_wait_synced.py

{% if pg.version.major_num >= 1200 %}
{{ pg.data }}/standby.signal:
    file.managed:
        - user: postgres
        - group: postgres
        - require:
            - file: {{ pg.data }}
            - postgresql_cmd: pg-init
{% endif %}
