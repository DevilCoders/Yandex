/etc/yc/api-als/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/als/als.yaml
    - template: jinja
    - makedirs: True

/etc/logrotate.d/als:
  file.managed:
    - source: salt://{{ slspath }}/files/als/yc-api-als.logrotate
    - require:
      - yc_pkg: yc-api-gateway

/etc/systemd/system/yc-api-als.service:
  file.managed:
    - source: salt://{{ slspath }}/files/als/yc-api-als.service

/var/log/als:
  file.directory:
    - user: yc-api-gateway
    - require:
      - yc_pkg: yc-api-gateway

als:
  service.running:
    - name: yc-api-als
    - enable: True
    - watch:
      - file: /etc/yc/api-als/config.yaml
      - yc_pkg: yc-api-gateway
    - require:
      - file: /etc/yc/api-als/config.yaml
      - yc_pkg: yc-api-gateway