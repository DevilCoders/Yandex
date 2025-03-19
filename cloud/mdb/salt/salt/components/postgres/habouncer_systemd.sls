{% for c in range(salt['pillar.get']('data:pgbouncer:internal_count', 1)) %}
{% set bouncer_count="%02d"|format(c|int) %}
/etc/pgbouncer/pgbouncer_internal{{ bouncer_count }}.ini:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgbouncer.ini
        - defaults:
            internal_bouncer: {{ bouncer_count }}
        - watch_in:
            - service: pgbouncer
{% endfor %}

{% for c in range(salt['pillar.get']('data:pgbouncer:count')) %}
{% set bouncer_count="%02d"|format(c|int) %}
/etc/pgbouncer/pgbouncer{{ bouncer_count }}.ini:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgbouncer.ini
        - defaults:
            bouncer: {{ bouncer_count }}
        - watch_in:
            - service: pgbouncer
{% endfor %}
