{% from "components/postgres/pg.jinja" import pg with context %}
{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}

include:
    - .common
    - components.odyssey
    - .odyssey-restart

pgbouncer-pkg:
    pkg:
        - name: pgbouncer
        - purged
    cmd.run:
        - name: killall pgbouncer || true
        - onchanges:
            - pkg: pgbouncer-pkg
        - require:
            - file: /etc/init.d/pgbouncer

/etc/init.d/pgbouncer:
    file.absent:
        - require:
            - pkg: pgbouncer-pkg

odyssey_service:
{% if salt['pillar.get']('data:use_pgsync', True) %}
    service.disabled:
{% else %}
    service.running:
        - enable: True
{% endif %}
        - name: odyssey
        - require:
            - pkg: odyssey
            - file: /etc/odyssey/odyssey.conf
            - file: /usr/local/yandex/odyssey-restart.sh


{# When we create a new replica under maintenance mode in pgsync we want it to open when it's in sync with primary #}
{% if pg.is_replica and salt.pillar.get('force-pooler-start', False) %}
odyssey-force-start:
    cmd.run:
        - name: service odyssey start
        - require:
            - service: odyssey
            - cmd: odyssey-reload2
            - cmd: postgresql-synced
        - unless:
            - service odyssey status
{% endif %}
