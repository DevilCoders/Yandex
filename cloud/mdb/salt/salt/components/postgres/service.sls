{% from "components/postgres/pg.jinja" import pg with context %}

{% if not (salt.pillar.get('update-odyssey', False) or salt.pillar.get('update-postgresql', False)) %}
include:
    - .pkgs_ubuntu
{% endif %}

postgresql-service:
    service.running:
        - name: {{ pg.service }}
        - reload: True
        - enable: True
    cmd.wait:
        - name: >
            /usr/local/yandex/pg_wait_started.py
            -w {{ salt['pillar.get']('data:pgsync:recovery_timeout', '1200') }}
            -c {{ salt['pillar.get']('data:config:checkpoint_timeout', '5min') }}
            -m {{ pg.version.major }}
        - runas: postgres
        - group: postgres
        - watch:
            - service: postgresql-service
{% if not (salt.pillar.get('update-odyssey', False) or salt.pillar.get('update-postgresql', False)) %}
        - require:
            - all-pg-packages-are-ready
{% endif %}
