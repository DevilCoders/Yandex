{% from slspath + "/map.jinja" import server with context %}
{% if server.get('monitoring', False) %}
redis_monitoring_package:
  pkg.installed:
    - name: yandex-media-common-redis-monitoring

/etc/yandex/redis-health.yaml:
  file.managed:
    - user: {{ server.user }}
    - group: {{ server.group }}
    - mode: 0644
    - makedirs: True
    - dir_more: 0755
    - source: salt://{{ slspath }}/files/etc/yandex/redis-health.yaml.tmpl
    - template: jinja
    - context:
        data: {{ server.monitoring }}

{% for check in server.monitoring.get('monrun', []) %}
redis_{{check}}:
  monrun.present:
    - command: /usr/bin/redis-health -P {{ server.params.port }} -o monrun -c {{ check }}
    - execution_interval: 60
    - execution_timeout: 30
    - type: {{ salt['grains.get']('conductor:project').encode('utf8') }}
{% endfor %}

{% if server.monitoring.get('graphite', False) %}
/usr/lib/yandex-graphite-checks/enabled/redis:
  file.symlink:
    - target: /usr/bin/redis-health
{% endif %}

{% endif %}


