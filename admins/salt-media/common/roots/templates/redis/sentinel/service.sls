{% from slspath + "/map.jinja" import sentinel with context %}

/etc/init.d/redis-sentinel:
  file.managed:
    - source: {{ sentinel.init }}
    - user: root
    - group: root
    - mode: 0755

redis_sentinel:
  service.running:
    - name: {{ sentinel.service }}
    - enable: True
    - watch:
      - file: /etc/redis/sentinel.conf
