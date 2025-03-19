pushclient:
    pkg.installed:
        - name: yandex-push-client
        - version: '6.74.0'
        - prereq_in:
            - cmd: repositories-ready
{% if salt.pillar.get('data:pushclient:start_service', True) %}
    service.running:
        - name: statbox-push-client
        - watch:
            - pkg: pushclient
            - file: /etc/pushclient/authlog_parser.py
            - file: /etc/default/push-client
            - file: /etc/systemd/system/statbox-push-client.service
            - group: statbox-in-adm-group
        - require:
            - pkg: pushclient
            - user: statbox-user
            - file: /etc/pushclient/logrotate.conf
            - file: /etc/systemd/system/statbox-push-client.service
{% else %}
    service.dead:
        - name: statbox-push-client
{% endif %}

{% if salt.pillar.get('data:pushclient:start_service', True) %}
pushclient-service-enable:
    service.enabled:
        - name: statbox-push-client
        - require:
            - pkg: pushclient
            - user: statbox-user
            - group: statbox-in-adm-group
{% endif %}

/etc/systemd/system/statbox-push-client.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/statbox-push-client.service
        - template: jinja
        - onchanges_in:
            - module: systemd-reload
        - require:
            - pkg: pushclient

pushclient-parser-pkgs:
    pkg.installed:
        - pkgs:
            - python3-dateutil

/etc/pushclient/logrotate.conf:
    file.absent

/etc/logrotate.d/pushclient:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - require:
            - pkg: pushclient

/etc/default/push-client:
    file.managed:
        - source: salt://{{ slspath }}/conf/default
        - template: jinja
        - makedirs: True
        - require:
            - pkg: pushclient

/etc/cron.d/pushclient-logrotate:
    file.absent

/etc/cron.yandex/wd-pushclient.sh:
    file.absent

{% if salt.pillar.get('data:pushclient:start_service', True) %}
/etc/cron.yandex/wd-pushclient.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/wd-pushclient.py
        - mode: 755
        - require:
            - file: /etc/default/push-client

/etc/cron.d/wd-pushclient:
    file.managed:
        - source: salt://{{ slspath }}/conf/wd-pushclient.cron
        - template: jinja
        - mode: 644
        - require:
            - file: /etc/cron.yandex/wd-pushclient.py
{% else %}
/etc/cron.yandex/wd-pushclient.py:
    file.absent

/etc/cron.d/wd-pushclient:
    file.absent
{% endif %}

/etc/init.d/statbox-push-client:
    file.managed:
        - source: salt://{{ slspath }}/conf/statbox-push-client.init
        - require:
            - pkg: pushclient
        - require_in:
            - service: pushclient

/etc/pushclient/authlog_parser.py:
    file.managed:
        - source: salt://components/pushclient/conf/authlog_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient

statbox-user:
  user.present:
    - name: statbox
    - gid_from_name: True
    - createhome: True
    - home: /var/lib/push-client
    - empty_password: True
    - shell: /bin/sh
    - system: True

statbox-in-adm-group:
    group.present:
        - name: adm
        - addusers:
            - statbox
        - system: True
        - require:
            - user: statbox-user

/var/log/statbox:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - pkg: pushclient
            - user: statbox-user
        - recurse:
            - user
            - group

{% if salt['pillar.get']('data:use_monrun', True) %}
/usr/local/yandex/monitoring/pushclient.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/monitoring.py
        - mode: 0755

/etc/monrun/conf.d/pushclient.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/monrun.conf
        - template: jinja
        - require:
            - file: /usr/local/yandex/monitoring/pushclient.py
        - watch_in:
            - cmd: monrun-jobs-update
{% else %}
/usr/local/yandex/telegraf/scripts/pushclient.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/monitoring.py
        - makedirs: True
        - mode: 0755
{% endif %}

/etc/pushclient/log_parser.py:
    file.managed:
        - source: salt://components/pushclient/parsers/log_parser.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: pushclient
        - watch_in:
            - service: pushclient

statbox-in-monitor-group:
    group.present:
        - name: monitor
        - gid: 1090
        - addusers:
            - statbox
        - system: True
        - watch_in:
            - service: pushclient
        - require_in:
            - service: pushclient
        - require:
            - user: statbox-user
