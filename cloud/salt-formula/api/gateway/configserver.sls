/etc/yc/api-configserver/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/configserver/configserver.yaml
    - template: jinja
    - makedirs: True

/etc/yc/api-configserver/gateway-services.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/configserver/gateway-services.yaml
    - template: jinja
    - makedirs: True
    - watch_in:
      - service: yc-api-gateway

/etc/yc/api-configserver/envoy-resources.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/configserver/envoy-resources.yaml
    - template: jinja
    - makedirs: True
    - watch_in:
      - service: envoy

/var/log/configserver:
  file.directory:
    - user: yc-api-gateway
    - require:
      - yc_pkg: yc-api-gateway

/etc/systemd/system/yc-api-configserver.service:
  file.managed:
    - source: salt://{{ slspath }}/files/configserver/yc-api-configserver.service

configserver:
  service.running:
    - name: yc-api-configserver
    - enable: True
    - watch:
      - file: /etc/yc/api-configserver/config.yaml
      - file: /etc/yc/api-configserver/gateway-services.yaml
      - file: /etc/yc/api-configserver/envoy-resources.yaml
      - file: /etc/systemd/system/yc-api-configserver.service
      - yc_pkg: yc-api-gateway

/etc/logrotate.d/configserver:
  file.managed:
    - source: salt://{{ slspath }}/files/configserver/yc-api-configserver.logrotate
    - require:
      - yc_pkg: yc-api-gateway
