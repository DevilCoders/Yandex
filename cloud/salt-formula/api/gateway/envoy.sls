# TODO: monitorings

/etc/envoy/ssl/certs/yc-api-gateway.crt:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/stub-certificate.crt
    - makedirs: True
    - replace: False
    - user: root
    - mode: 400

/etc/envoy/ssl/private/yc-api-gateway.key:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/stub-certificate.key
    - makedirs: True
    - replace: False
    - user: root
    - mode: 400

/etc/envoy/envoy.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/envoy.yaml
    - template: jinja
    - makedirs: True

/usr/local/bin/envoy_wrapper.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/envoy_wrapper.sh
    - user: root
    - mode: 755

/etc/systemd/system/envoy.service:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/envoy.service

envoy:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /etc/envoy/ssl/certs/yc-api-gateway.crt
      - file: /etc/envoy/ssl/private/yc-api-gateway.key
      - file: /etc/systemd/system/envoy.service
      - file: /usr/local/bin/envoy_wrapper.sh
      - file: /etc/envoy/envoy.yaml
      - yc_pkg: yc-api-gateway
    - require:
      - yc_pkg: yc-api-gateway
      - service: yc-api-als
      - service: yc-api-configserver

/etc/logrotate.d/envoy:
  file.managed:
    - source: salt://{{ slspath }}/files/envoy/envoy.logrotate
    - require:
      - yc_pkg: yc-api-gateway

/etc/cron.d/logrotate:
  file.managed:
    - source: salt://{{ slspath }}/files/stats/logrotate.crond
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-api-gateway
