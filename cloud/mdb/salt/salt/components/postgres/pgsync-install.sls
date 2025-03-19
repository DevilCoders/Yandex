{% from "components/postgres/pg.jinja" import pg with context %}

/var/log/pgsync:
    file.directory:
        - user: postgres
        - group: postgres

/var/run/pgsync:
    file.directory:
        - user: postgres
        - group: postgres

{% if salt['pillar.get']('restore-from:cid') %}
init-pgsync-for-restored-cluster:
    cmd.run:
        - name: timeout 5 pgsync-util initzk {{ salt['grains.get']('fqdn')  }}
        - require:
            - file: /etc/pgsync.conf
            - service: postgresql-service
        - require_in:
            - service: pgsync
{% endif %}

{% if salt['pillar.get']('data:use_pgsync', True) %}
yamail-pgsync:
    pkg.installed:
        - prereq_in:
            - cmd: repositories-ready
        - pkgs:
            - yamail-pgsync: {{ pg.pgsync_version }}
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
            - python-daemon
            - python-lockfile
        - require:
            - file: {{ pg.prefix }}/.pgpass
{%   if salt['pillar.get']('data:start_pgsync', True) %}
        - watch_in:
            - file: /etc/pgsync.conf
{%   endif %}

{% endif %}

include:
    - .configs.pgpass
{% if salt['pillar.get']('data:start_pgsync', True) %}
    - .pgsync

extend:
    pgsync:
        service.running:
            - require:
                - service: postgresql-service
                - file: /var/log/pgsync
                - file: /var/run/pgsync
{% if pg.is_master and salt['pillar.get']('data:pgsync:use_lwaldump', 'yes') == 'yes' %}
                - mdb_postgresql: pg-extension-lwaldump-postgres
{% endif %}
            - watch:
                - pkg: yamail-pgsync

pgsync-enable:
    service.enabled:
        - name: pgsync
        - require:
            - pkg: yamail-pgsync

/lib/systemd/system/pgsync.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgsync.service
        - require_in:
            - service: pgsync
        - onchanges_in:
            - module: systemd-reload

/etc/cron.d/wd-pgsync:
    file.absent:
        - require:
            - pkg: yamail-pgsync
        - require_in:
            - service: pgsync

/etc/cron.yandex/wd_pgsync.py:
    file.absent:
        - require:
            - pkg: yamail-pgsync
        - require_in:
            - service: pgsync

/etc/init.d/pgsync:
    file.absent:
        - require:
            - pkg: yamail-pgsync
        - require_in:
            - service: pgsync
{% else %}
pgsync:
    service.dead:
        - enable: false

/etc/cron.d/wd-pgsync:
    file.absent:
        - require:
            - pkg: yamail-pgsync
        - require_in:
            - service: pgsync
{% endif %}

{% if salt['pillar.get']('data:auto_resetup', False) %}
/etc/cron.yandex/pg_resetup.py:
    file.managed:
        - mode: 755
        - source: salt://{{ slspath }}/conf/pg_resetup.py

/etc/cron.d/pg_resetup:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/cron.d/pg_resetup
        - mode: 644
{% endif %}

{% if salt['pillar.get']('data:pgsync:plugins:upload_wals', True) %}
/etc/pgsync/plugins/upload_wals.py:
    file.symlink:
{% if pg.pgsync_version == '276-9be76ed' %}
        - target: /usr/local/lib/python2.7/dist-packages/pgsync/plugins/upload_wals.py
{% else %}
        - target: /opt/yandex/pgsync/lib/python3.6/site-packages/pgsync/plugins/upload_wals.py
{% endif %}
        - require:
            - pkg: yamail-pgsync
        - require_in:
            - service: pgsync
{% endif %}
