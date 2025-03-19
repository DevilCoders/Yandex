{%- set env = grains['yandex-environment'] -%}
{% set unit_config = pillar.get('rbtorrent', {}) %}
{% set app_config = pillar.get('rbtorrent_config', {}) %}
{% set check_config = pillar.get('rbtorrent_check') %}
{% set upstream_local = 'localhost:%s' | format(app_config.get('HTTP_LISTEN_PORT', 9999)) %}
{% if env == 'production' %}
    {% set upstream = salt['conductor']['groups2hosts']('elliptics-rbtorrent')  | map('regex_replace', '$', ':9999') | list %}
{% endif %}
{% set server_name = unit_config.get('server_name') %}
{% set do_cleanup = unit_config.get('do_cleanup', False) %}
{% set cleanup_lock_name = unit_config.get('cleanup_lock_name', grains['conductor']['group'] + '-cleanup') %}
{% set zk_flock_hosts = unit_config.get('zk_flock_hosts', []) %}


include:
  - units.ubic
  - units.iface-ip-conf
  - templates.unistat-lua-base

{% from 'units/ubic/macro.sls' import ubic_service %}
{{ ubic_service('rbtorrent', '/usr/share/python/rbtorrent/bin/rbtorrent-standalone -c /etc/yandex/rbtorrent.yaml') }}

nginx:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/*

/etc/yandex/rbtorrent.yaml:
  file.serialize:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - watch_in:
      - service: rbtorrent
    - dataset_pillar: rbtorrent_config
    - formatter: yaml

/etc/nginx/sites-enabled/10-rbtorrent.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - watch_in:
        - service: nginx  # FIXME
    - source: salt://{{ slspath }}/files/nginx/10-rbtorrent.conf
    - context:
        {% if env == 'production' %}
            {% set upstream = salt['conductor']['groups2hosts']('elliptics-rbtorrent')  | map('regex_replace', '$', ':9999') | list %}
        upstream: {{ upstream }}
        {% endif %}
        upstream_local: {{ upstream_local }}
        server_name: {{ server_name }}
    - template: jinja

/etc/nginx/conf.d/01-rbtorrent-tskv.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - watch_in:
        - service: nginx  # FIXME
    - contents: |
        tskv_log_format rbtorrent-access-log 'request_id=$request_id scheme=$scheme upstream_addr=$upstream_addr upstream_status=$upstream_status bytes_received=$request_length bytes_sent=$bytes_sent content_length=$http_content_length upstream_response_time=$upstream_response_time request_time=$request_time';
        tskv_log /var/log/nginx/rbtorrent-tskv.log rbtorrent-access-log;

/etc/logrotate.d/rbtorrent-nginx-tskv:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        /var/log/nginx/rbtorrent-tskv.log
        {
                size 1024M
                missingok
                rotate 12
                compress
                delaycompress
                notifempty
                sharedscripts
                postrotate
                        kill -USR1 `cat /var/run/nginx.pid` && sleep 2
                endscript
        }

/usr/local/bin/skybone-mds-cleanup.py:
  file.absent

/etc/distributed-flock.json:
  file.serialize:
    - user: root
    - group: root
    - mode: 644
    - dataset:
        host: {{ zk_flock_hosts }}
        timeout: 5
        app_id: {{ grains['conductor']['group']}}
        path: '/var/log/zk-flock'
    - formatter: json

/var/log/rbtorrent-cleanup:
  file.directory:
    - user: root
    - group: root
    - mode: 755

/etc/logrotate.d/rbtorrent-cleanup:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        /var/log/rbtorrent-cleanup/cleanup.log
        {
                size 100M
                missingok
                rotate 12
                compress
                delaycompress
                notifempty
        }

{% if do_cleanup %}
/etc/cron.d/rbtorrent-cleanup:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        13 * * * * root sleep $((RANDOM\%59)) && zk-flock -w 5 {{cleanup_lock_name}} '/usr/share/python/rbtorrent/bin/rbtorrent-cleanup -c /etc/yandex/rbtorrent.yaml --skybone-proxy-timeout 60' 2>> /var/log/rbtorrent-cleanup/cleanup.log
{% else %}
/etc/cron.d/rbtorrent-cleanup:
  file.absent
{% endif %}


rbtorrent check:
  file.managed:
    - name: '/usr/local/bin/rbtorrent-check.py'
    - source: salt://{{ slspath }}/files/rbtorrent-check.py
    - user: root
    - group: root
    - mode: 755
  monrun.present:
    - name: rbtorrent-check
    - command: '/usr/local/bin/rbtorrent-check.py {{check_config['bucket']}} {{check_config['key']}} {{check_config['md5']}}'
    - execution_interval: 191
    - execution_timeout: 100

skybone check:
  file.managed:
    - name: '/usr/local/bin/skybone-check.sh'
    - source: salt://{{ slspath }}/files/skybone-check.sh
    - user: root
    - group: root
    - mode: 755
  monrun.present:
    - name: skybone-check
    - command: '/usr/local/bin/skybone-check.sh'
    - execution_interval: 180
    - execution_timeout: 30
