{% set pgbouncer_version = '1007-a122af8' %}
{% set osrelease = salt['grains.get']('osrelease') %}

{# prefer the version from data:versions, if set #}
{% set pgbouncer_version = salt['pillar.get']('data:versions:pgbouncer:package_version', pgbouncer_version) %}

odyssey:
    pkg:
        - purged
    cmd.run:
        - name: killall odyssey || true
        - onchanges:
            - pkg: odyssey

/var/log/postgresql/pgbouncer.log:
    file.managed:
        - user: postgres
        - group: postgres
        - require:
            - pkg: pgbouncer-pkg
        - require_in:
            - service: pgbouncer

pgbouncer-pkg:
    pkg.installed:
        - name: pgbouncer
        - version: {{ pgbouncer_version }}
        - require_in:
           - file: /etc/pgbouncer/pgbouncer.ini
           - file: /etc/pgbouncer/userlist.txt

include:
    - .pgbouncer

extend:
    pgbouncer:
{% if salt['pillar.get']('data:use_pgsync', True) %}
        service.disabled:
{% else %}
        service.running:
{% endif %}
            - require:
                - pkg: pgbouncer-pkg
                - file: /etc/pgbouncer/pgbouncer.ini
                - file: /etc/pgbouncer/userlist.txt
                - file: /etc/default/pgbouncer

/etc/default/pgbouncer:
    file.managed:
        - source: salt://{{ slspath }}/conf/pgbouncer-default-ubuntu


{% if salt['pillar.get']('data:pgbouncer:count', 1)==1 %}
/etc/init.d/pgbouncer:
    file.managed:
        - source: salt://{{ slspath }}/conf/pgbouncer.init.ubuntu
        - mode: 755
        - user: root
        - group: root
        - require:
            - pkg: pgbouncer-pkg
        - require_in:
            - service: pgbouncer
{% endif %}

{% if osrelease == '18.04' %}
systemd-disable-all-bouncers:
    cmd.run:
        - name: systemctl -t service -a | egrep -o 'pgbouncer@[^\.]+' | xargs --no-run-if-empty systemctl disable
        - onchanges_in:
            - module: systemd-reload
        - onchanges:
            - file: /lib/systemd/system/pgbouncer.service

stop-pgbouncers:
    cmd.run:
        - name: service pgbouncer stop
        - require:
            - file: /lib/systemd/system/pgbouncer.service
            - module: systemd-reload
        - onchanges:
            - cmd: systemd-disable-all-bouncers

start-pgbouncers:
    cmd.run:
        - name: service pgbouncer start
        - onchanges:
            - cmd: stop-pgbouncers
        - require:
            - module: systemd-reload

{%   if salt['pillar.get']('data:pgbouncer:count', 1) == 1 %}
/lib/systemd/system/pgbouncer.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/pgbouncer.service
        - template: jinja
        - onchanges_in:
            - module: systemd-reload
{%   endif %}

{% endif %}
