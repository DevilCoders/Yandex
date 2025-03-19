{%- from slspath + "/map.jinja" import mm_vars with context -%}
{%- set federation = pillar.get('mds_federation', None) -%}

include:
  {% if not federation or federation == 1 -%}
  - .nscfg
  - .resizer
  - .stat-reporter
  - .resource-provider
  {%- endif %}
  - .monolith
  - .job-processor
  - .scheduler
  - .inventory
  - .state-builder
  - .drooz
  - .run-cycle-resize-jobs
  - units.zcore
  - templates.unistat-lua

yandex-internal-root-ca-pkg:
  pkg.installed:
      - name: yandex-internal-root-ca

/etc/elliptics/mastermind.conf:
  yafile.managed:
  {% if pillar.get('is_cloud', False) %}
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind-cloud.conf
  {% endif %}
  {% if pillar.get('is_collector', False) %}
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind-collector.conf
  {% endif %}
    - template: jinja
    - context:
      vars: {{ mm_vars }}

/etc/nginx/conf.d/01-mm-access-log-tskv.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/conf.d/01-mm-access-log-tskv.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - template: jinja
    {% if not federation or federation == 1 -%}
    - watch_in:
        - service: nginx_nscfg
    {%- endif %}

{% if pillar.get('is_collector', False) %}
collector-status:
  monrun.present:
    - command: RESULT=$(timeout 30 mastermind collector ping 2>&1); [[ $RESULT == "ok" ]] && echo '0;OK' || (echo "2;"; echo "${RESULT}" |tail -n1 ) | xargs
    - execution_interval: 60
    - execution_timeout: 30
    - type: mastermind

/etc/cron.d/config-mds-mastermind:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/config-mds-mastermind
    - template: jinja

/var/tmp/disable_mastermind_monrun:
  file.managed:
    - contents: ''
    - contents_newline: False
{% else %}
/etc/cron.d/config-mds-mastermind:
  file.absent
{% endif %}

/etc/ubic/service/flowmastermind.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/flowmastermind.json

# Костыль. Мне стыдно
/etc/syslog-ng/syslog-ng-default.tpl:
  file.managed:
   - source: salt://{{ slspath }}/files/etc/syslog-ng/syslog-ng-default.tpl
   - user: root
   - group: root
   - mode: 644

/usr/bin/monrun-resizer-and-reporter.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/bin/monrun-resizer-and-reporter.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: true

sa-notifications:
  file.managed:
    - source: salt://{{ slspath }}/files/bin/sa-notifications.py
    - name: "/usr/bin/sa-notifications.py"
    - user: root
    - group: root
    - mode: 755
    - makedirs: true
    - template: jinja

/usr/bin/trash_couples_finder.py:
  file.managed:
    - source: salt://{{ slspath }}/files/bin/trash_couples_finder.py
    - user: root
    - group: root
    - mode: 755
    - makedirs: true

/usr/bin/retry_jobs.py:
  file.managed:
    - source: salt://{{ slspath }}/files/bin/retry_jobs.py
    - user: root
    - group: root
    - mode: 755
    - makedirs: true

# Fix me
cocaine-user:
  user.present:
    - name: cocaine

/var/log/cocaine-core:
  file.directory:
    - mode: 755
    - user: cocaine
    - group: root
    - makedirs: True

/var/cache/mastermind:
  file.directory:
    - mode: 1777
    - user: root
    - group: root
    - makedirs: True

photo-cocaine-conf:
  file.managed:
    - name: /etc/logrotate.d/photo-cocaine-conf
    - contents: |
        /var/log/cocaine-core/*.log
        {
            daily
            maxsize 2048M
            missingok
            rotate 35
            compress
            compresscmd /usr/bin/pzstd
            compressoptions -8 -p1
            compressext .zst
            notifempty
            dateext
            dateformat .%Y%m%d-%s
            create 644 cocaine adm
        }
    - user: root
    - group: root
    - mode: 644

/etc/mds-logbackup/config-lifecycle.json:
  file.managed:
    - source: salt://{{ slspath }}/files/config-lifecycle.json
    - user: root
    - group: root
    - makedirs: true
