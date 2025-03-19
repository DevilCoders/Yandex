{% set unit = 'cocaine-srw' %}
{% set cluster = pillar.get("cluster") %}

{% set cocaine_srw_755 =
   [ "/usr/lib/yandex-3132-cgi/cocaine-tool-info",
     "/usr/local/bin/cocaine-isolate-daemon.py",
     "/usr/local/bin/cocaine-depth.py",
     "/usr/local/bin/cocaine-respoolfailed.sh",
     "/usr/local/bin/cocaine-warmup.py",
     "/usr/local/bin/cocaine-app-status.py",
     "/usr/local/bin/vlan688_check.sh",
     "/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py"
   ]
 %}

{% for file in cocaine_srw_755 %}
{{file}}:
  yafile.managed:
    - source: salt://templates/{{unit}}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% set cluster = pillar.get('cluster') %}


vlan688:
  monrun.present:
    - command: /usr/local/bin/vlan688_check.sh
    - execution_interval: 120
    - execution_timeout: 60
    - type: srw

cocaine-depth:
  monrun.present:
    - command: /usr/local/bin/cocaine-depth.py
    - execution_interval: 60
    - execution_timeout: 180
    - type: srw

cocaine-isolate-daemon:
  monrun.present:
    - command: /usr/local/bin/cocaine-isolate-daemon.py
    - execution_interval: 60
    - execution_timeout: 180
    - type: srw

cocaine-app-status_10071:
  monrun.present:
    - command: /usr/local/bin/cocaine-app-status.py 10071
    - execution_interval: 60
    - execution_timeout: 180
    - type: srw

cocaine-respoolfailed:
  cron.present:
    - name: /usr/local/bin/cocaine-respoolfailed.sh
    - user: root
    - identifier: respoolfailed
    - minute: '*/30'

/var/cache/unistorage-acl:
  file.directory:
    - user: cocaine
    - mode: 755
    - makedirs: True

