include:
 - units.postgresql
 - units.distributed-flock
 - units.juggler-checks.common
 - units.juggler-checks.irrd

irrd-pkgs:
  pkg.installed:
    - pkgs:
      - irrd
      - zk-flock
      - go-zk-client
      - whois

/etc/irrd.yaml:
  file.managed:
    - source: salt://{{slspath}}/files/irrd.yaml
    - template: jinja
    - user: irrd
    - group: irrd
    - mode: 600

/etc/irrd-log-config.py:
  file.managed:
    - source: salt://{{slspath}}/files/irrd-log-config.py
    - user: irrd
    - group: irrd
    - mode: 600
  cmd.run:
    - name: /opt/irrd/bin/pip install -i https://pypi.yandex-team.ru/simple/ python-json-logger
    - unless: /opt/irrd/bin/pip freeze | grep python-json-logger
