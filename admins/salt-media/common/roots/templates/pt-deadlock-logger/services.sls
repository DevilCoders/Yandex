{% if grains['init'] in ['systemd'] %}
pt-dl-systemd-reload:
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: /etc/init.d/pt-deadlock-logger
{% endif %}

ubic-pt-deadlock-logger:
  service.running:
    - name: pt-deadlock-logger
    - enable: True
    - sig: 'perl /usr/bin/pt-deadlock-logger --config'
    - watch:
      - file: /etc/yandex/percona-toolkit/pt-deadlock-logger/config
      - file: /etc/ubic/service/pt-deadlock-logger
