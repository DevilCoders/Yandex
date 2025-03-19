{% from slspath + "/map.jinja" import memcached with context %}
include:
  - .services

{% for d in memcached.directories %}
{{d}}:
  file.directory:
    - name: {{d}}
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
{% endfor %}

/etc/memcached.conf:
  file.managed:
    - source: salt://templates/memcached/files/etc/memcached.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - replace: true

/etc/memcached-check.conf:
  file.managed:
    - source: salt://templates/memcached/files/etc/memcached-check.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - replace: true

{% if memcached.mcrouter is defined %}
{% if salt["grains.get"]("osrelease") >= 16.04 %}
{% set osname = salt["grains.get"]("oscodename") %}
mcrouter_systemd:
  file.managed:
    - name: /etc/systemd/system/mcrouter.service
    - source: salt://{{ slspath }}/files/etc/systemd/system/mcrouter.service
    - makedir: True
    - require_in:
      - service: mcrouter_service

service.systemctl_reload:
  module.run:
    - onchanges:
      - file: /etc/systemd/system/mcrouter.service
{% endif %}

/etc/mcrouter/mcrouter.conf:
  file.managed:
    - source: 
      - salt://mcrouter/files/etc/mcrouter/{{ memcached.mcrouter_cfgname }}
      - salt://templates/memcached/files/etc/mcrouter/mcrouter.conf
    - template: jinja
    - replace: true
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mcrouter
      - file: /var/log/mcrouter
      - file: /var/spool/mcrouter

mcrouter_monrun:
  file.managed:
    - name: /etc/monrun/conf.d/mcrouter-check.conf
    - source: salt://templates/memcached/files/etc/mcrouter/mcrouter.monrun.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mcrouter
      - file: /var/log/mcrouter
      - file: /var/spool/mcrouter
  cmd.run:
    - name: /usr/sbin/regenerate-monrun-tasks
    - onchanges:
      - file: /etc/monrun/conf.d/mcrouter-check.conf

/usr/sbin/mcrouter_errors.sh:
  file.managed:
    - source: salt://templates/memcached/files/usr/sbin/mcrouter_errors.sh
    - template: jinja
    - replace: true
    - user: root
    - group: root
    - mode: 750

mcrouter_errors_monrun:
  file.managed:
    - name: /etc/monrun/conf.d/mcrouter_errors.conf
    - source: salt://templates/memcached/files/etc/mcrouter/mcrouter_errors_monrun.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mcrouter
      - file: /var/log/mcrouter
      - file: /var/spool/mcrouter
  cmd.run:
    - name: /usr/sbin/regenerate-monrun-tasks
    - onchanges:
      - file: /etc/monrun/conf.d/mcrouter_errors.conf

/etc/default/mcrouter:
  file.managed:
    - source: salt://templates/memcached/files/etc/mcrouter/mcrouter_default
    - template: jinja
    - replace: true
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mcrouter
      - file: /var/log/mcrouter
      - file: /var/spool/mcrouter

/usr/lib/yandex-graphite-checks/enabled/mcrouter.sh:
  file.managed:
    - source: salt://templates/memcached/files/usr/lib/yandex-graphite-checks/enabled/mcrouter.sh
    - template: jinja
    - replace: true
    - user: root
    - group: root
    - mode: 750
{% endif %}

/usr/lib/yandex-graphite-checks/enabled/memcached.sh:
  file.managed:
    - source: salt://templates/memcached/files/usr/lib/yandex-graphite-checks/enabled/memcached.sh
    - template: jinja
    - replace: true
    - user: root
    - group: root
    - mode: 750

