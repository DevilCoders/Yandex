{{ sls }}-selfdns-client:
  yc_pkg.installed:
    - pkgs:
      - selfdns-client
  file.managed:
    - name: /etc/yandex/selfdns-client/default.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/yandex-selfdns-client_default.conf
    - require:
      - yc_pkg: {{ sls }}-selfdns-client
  cmd.wait:
    - name: /usr/bin/selfdns-client --debug --terminal 1>> /var/log/yandex-selfdns-client/run_from_salt.log 2>&1
    - runas: selfdns
    - cwd: /tmp/
    - watch:
      - file: {{ sls }}-selfdns-client
      - service: networking_started
