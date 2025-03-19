/lib/systemd/system/pgbouncer.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/habouncer.service
        - template: jinja
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/pgbouncer@.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/habouncer_template.service
        - template: jinja
        - require:
            - file: /lib/systemd/system/pgbouncer.service
        - onchanges_in:
            - module: systemd-reload

{% for c in range(salt['pillar.get']('data:pgbouncer:internal_count', 1)) %}
{% set bouncer_count="%02d"|format(c|int) %}
systemd-enable-pgbouncer_internal{{ bouncer_count }}:
    service.enabled:
        - name: pgbouncer@_internal{{ bouncer_count }}
        - require:
            - file: /lib/systemd/system/pgbouncer@.service
            - file: /lib/systemd/system/pgbouncer.service
        - onchanges_in:
            - module: systemd-reload
            - cmd: stop-pgbouncers
{% endfor %}


{% for c in range(salt['pillar.get']('data:pgbouncer:count')) %}
{% set bouncer_count="%02d"|format(c|int) %}
systemd-enable-pgbouncer{{ bouncer_count }}:
    service.enabled:
        - name: pgbouncer@{{ bouncer_count }}
        - require:
            - file: /lib/systemd/system/pgbouncer@.service
            - file: /lib/systemd/system/pgbouncer.service
        - onchanges_in:
            - module: systemd-reload
{% endfor %}

include:
    - .habouncer_systemd
