include:
  - units.nginx_conf
  - units.distributed-flock
  - units.juggler-checks.common
  - units.juggler-checks.matilda

matilda-pkgs:
  pkg:
    - installed
    - pkgs:
      - groppy
      - matilda-clickhouse-proxy
      - matilda-dicts=1.r43ec44acd6cdaa60106b3b7972a17ff55111e1a8
      - matilda-netdb
      - matilda-netdb-agent
      - matilda-server
      - python3-frontilda
      - yandex-coroner
      - yandex-udp-balancer
      - yandex-udp-samplicator-base

/etc/yandex-udp-balancer/balancer.conf:
  file.managed:
    - source: salt://files/nocdev-matilda/etc/yandex-udp-balancer/balancer.conf
    - template: jinja
    - makedirs: True

/etc/matilda/:
  file.recurse:
    - source: salt://files/nocdev-matilda/etc/matilda/
    - template: jinja

/etc/matilda/abc-token:
  file.managed:
    - contents: {{ pillar['sec']['abc-token'] }}
    - template: jinja

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-matilda/etc/nginx/sites-enabled/
    - template: jinja

/etc/matilda-clickhouse-proxy/config.yml:
  file.managed:
    - source: salt://files/nocdev-matilda/etc/matilda-clickhouse-proxy/config.yml
    - template: jinja

/etc/netconfig.d/01-matilda.py:
  file.managed:
    - source: salt://files/nocdev-matilda/etc/netconfig.d/01-matilda.py
    - template: jinja

/etc/frontilda/frontilda.yml:
  file.managed:
    - source: salt://files/nocdev-matilda/etc/frontilda/frontilda.yml
    - template: jinja
    - makedirs: True

/etc/groppy.yml:
  file.managed:
    - source: salt://files/nocdev-matilda/etc/groppy.yml
    - template: jinja
    - makedirs: True

{% for service in ['matilda-nfsense-yadscp', 'matilda-sflow' , 'matilda-ipfix', 'matilda-nfsense-decap', 'matilda-nfsense-nat', 'matilda-nfsense', 'matilda-nfsense-fct', 'matilda-clickhouse-proxy', 'yandex-udp-balancer', 'frontilda-server', 'groppy-server'] %}
{{ service }}:
  service.running:
    - enable: True
    - reload: True
{%endfor%}
