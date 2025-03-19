postgresql-habouncer-req:
    test.nop

postgresql-habouncer-ready:
    test.nop

{% for c in range(salt['pillar.get']('data:pgbouncer:count')) %}
{% set bouncer_count="%02d"|format(c|int) %}
/etc/pgbouncer/pgbouncer{{ bouncer_count }}.ini:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgbouncer.ini
        - defaults:
            bouncer: {{ bouncer_count }}
        - require:
            - test: postgresql-habouncer-req
        - require_in:
            - test: postgresql-habouncer-ready

pgbouncer{{ bouncer_count }}-reload:
    cmd.wait:
        - name: pid=$(supervisorctl pid pgbouncer_external:pgbouncer_external{{ bouncer_count }}); if [ "$pid" != "0" ]; then echo "Reloading $pid"; kill -HUP "$pid"; fi
        - watch:
            - file: /etc/pgbouncer/pgbouncer{{ bouncer_count }}.ini
        - require:
            - postgresql-habouncer-req
        - require_in:
            - test: postgresql-habouncer-ready
{% endfor %}

{% for c in range(salt['pillar.get']('data:pgbouncer:internal_count', 1)) %}
{% set bouncer_count="%02d"|format(c|int) %}
/etc/pgbouncer/pgbouncer_internal{{ bouncer_count }}.ini:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgbouncer.ini
        - defaults:
            internal_bouncer: {{ bouncer_count }}
        - require:
            - postgresql-habouncer-req
        - require_in:
            - test: postgresql-habouncer-ready

pgbouncer_internal{{ bouncer_count }}-reload:
    cmd.wait:
{% if salt['pillar.get']('data:unmanaged_dbs', {}) %}
        - name: supervisorctl restart pgbouncer_internal{{ bouncer_count }}
{% else %}
        - name: pid=$(supervisorctl pid pgbouncer_internal:pgbouncer_internal{{ bouncer_count }}); if [ "$pid" != "0" ]; then echo "Reloading $pid"; kill -HUP "$pid"; fi
{% endif %}
        - require:
            - postgresql-habouncer-req
        - require_in:
            - test: postgresql-habouncer-ready
        - watch:
            - file: /etc/pgbouncer/pgbouncer_internal{{ bouncer_count }}.ini
{% endfor %}
