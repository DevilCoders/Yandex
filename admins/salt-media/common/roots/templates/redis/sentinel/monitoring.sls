{% from slspath + "/map.jinja" import sentinel with context %}
{% if sentinel.get('monitoring', False) %}
sentinel_monitoring_package:
  pkg.installed:
    - name: yandex-media-common-redis-monitoring

/etc/yandex/redis-sentinel-health.yaml:
  file.managed:
    - user: {{ sentinel.user }}
    - group: {{ sentinel.group }}
    - mode: 0644
    - makedirs: True
    - dir_mode: 0755
    - source: salt://{{ slspath }}/files/etc/yandex/redis-health.yaml.tmpl
    - template: jinja
    - context:
        data: {{ sentinel.monitoring }}

{% for check in sentinel.monitoring.get('monrun', []) %}
sentinel_{{check}}:
  monrun.present:
    - command: /usr/bin/redis-health --config /etc/yandex/redis-sentinel-health.yaml -P {{ sentinel.params.port }} -o monrun -c {{ check }}
    - execution_interval: 60
    - execution_timeout: 30
    - type: {{ salt['grains.get']('conductor:project').encode('utf8') }}
{% endfor %}

{% if sentinel.monitoring.get('graphite', False) %}
/usr/lib/yandex-graphite-checks/enabled/sentinel:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/lib/yandex-graphite-checks/enabled/sentinel
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - dir_mode: 0755
    - template: jinja
    - context:
        port: {{ sentinel.params.port }}
        config: "/etc/yandex/redis-sentinel-health.yaml"
{% endif %}

{% endif %}


