include:
  - .configs
  - .clickphite
  - .metrics

# FIXME: hardcode inside
/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/monrun/conf.d/

/usr/local/bin/logbroker-tool:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/logbroker-tool
    - mode: 755

/var/log/yandex/logshatter:
  file.symlink:
    - target: /var/log/logshatter
    - user: logshatter
    - group: nogroup

logshatter:
  service:
    - running
    - enable: True
