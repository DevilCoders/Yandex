/etc/valve/valve.yaml:
  file.managed:
    - source: salt://{{slspath}}/files/etc/valve/valve.yaml-{{grains["yandex-environment"]}}
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/usr/sbin/valve_check.py:
  file.managed:
    - source: salt://{{slspath}}/files/usr/sbin/valve_check.py
    - mode: 755

/usr/sbin/weight.sh:
  file.managed:
    - source: salt://{{slspath}}/files/usr/sbin/weight.sh
    - mode: 755

/etc/default/yasmagent:
  file.managed:
    - source: salt://{{slspath}}/files/etc/default/yasmagent

/usr/local/yasmagent/CONF/agent.valve.conf:
  file.managed:
    - source: salt://{{slspath}}/files/usr/local/yasmagent/CONF/agent.valve.conf
    - makedirs: True

/etc/valve/env:
  file.managed:
    - source: salt://{{slspath}}/files/etc/valve/env
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/logrotate.d/valve:
  file.managed:
    - source: salt://{{slspath}}/files/etc/logrotate.d/valve

{% if grains["yandex-environment"] == "testing" %}
/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{slspath}}/files/etc/nginx/nginx.conf-testing
{% endif %}

/etc/nginx/sites-enabled/30-valve.conf:
  file.managed:
    - source: salt://{{slspath}}/files/etc/nginx/sites-enabled/30-valve.conf-{{grains["yandex-environment"]}}
    - template: jinja

/etc/nginx/conf.d/99-accesslog.conf:
  file.managed:
    - source: salt://{{slspath}}/files/etc/nginx/conf.d/99-accesslog.conf

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://{{slspath}}/files/etc/logrotate.d/nginx

restart_valve:
  service.running:
    - name: valve
    - restart: true
    - watch:
      - file: /etc/nginx/ssl/key.valve.yandex.net.pem
      - file: /etc/nginx/ssl/cert.valve.yandex.net.pem

/etc/nginx/conf.d/01-accesslog.conf: # cleanup old configs
  file.absent
