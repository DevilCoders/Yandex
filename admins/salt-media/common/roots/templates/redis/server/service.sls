{% from slspath + "/map.jinja" import server with context %}

redis_server:
  service.running:
    - name: {{ server.service }}
    - enable: True
    - watch:
      - file: /etc/redis/redis.conf
