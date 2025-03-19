{% from "templates/proftpd/map.jinja" import proftpd with context %}

include:
  - templates.proftpd

proftpd_server:
  service:
    - name: {{ proftpd.service }}
    - enable: True
    - running

proftpd_conf:
  file:
    - managed
    - name: {{ proftpd.filename }}
    - source:
      - salt://files/{{ grains['conductor']['group'] }}{{ proftpd.filename }}
      - {{ proftpd.filesource }}
    - template: jinja
    - context:
      default_address: {{ proftpd.addr }}
      default_port: {{ proftpd.port }}
      default_root: {{ proftpd.rootpath }}
      default_user: {{ proftpd.user }}
      default_group: {{ proftpd.group }}
    - user: root
    - watch_in:
      - service: proftpd_server
