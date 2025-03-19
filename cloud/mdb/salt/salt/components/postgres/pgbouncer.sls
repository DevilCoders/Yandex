pgbouncer:
{% if salt['pillar.get']('data:use_pgsync', True) %}
    service.disabled
{% else %}
    service.running:
        - enable: True
{% endif %}

{% if salt['pillar.get']('data:unmanaged_dbs', {}) %}
pgbouncer-restart:
    cmd.wait:
        - name: service pgbouncer stop; service pgbouncer start || true
        - watch:
            - file: /etc/pgbouncer/pgbouncer.ini
        - require:
            - service: pgbouncer
{% elif salt['pillar.get']('data:pgbouncer:count', 1)==1 %}
pgbouncer-reload:
    cmd.wait:
        - name: service pgbouncer reload
        - watch:
            - file: /etc/pgbouncer/pgbouncer.ini
        - require:
            - service: pgbouncer
{% endif %}
