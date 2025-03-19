{% if grains['init'] in ['systemd'] %}
pt-fkel-systemd-reload:
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: /etc/init.d/pt-fk-error-logger
{% endif %}

ubic-pt-fk-error-logger:
  service.running:
    - name: pt-fk-error-logger
    - enable: True
    - sig: 'perl /usr/bin/pt-fk-error-logger --config'
    - watch:
      - file: /etc/yandex/percona-toolkit/pt-fk-error-logger/config
      - file: /etc/ubic/service/pt-fk-error-logger
