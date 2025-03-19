{%- import_yaml slspath+"/mon/api-gateway-envoy-in.yaml" as monitoring %}
{%- include "common/deploy_mon_scripts.sls" %}

/etc/yc/api-gateway/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/gateway/gateway.yaml
    - template: jinja
    - makedirs: True

/var/log/gateway:
  file.directory:
    - user: yc-api-gateway
    - require:
      - yc_pkg: yc-api-gateway

gateway:
  service.running:
    - name: yc-api-gateway
    - enable: True
    - watch:
      - file: /etc/yc/api-gateway/config.yaml
      - yc_pkg: yc-api-gateway
    - require:
      - file: /etc/yc/api-gateway/config.yaml
      - yc_pkg: yc-api-gateway
      - service: yc-api-configserver

/etc/logrotate.d/gateway:
  file.managed:
    - source: salt://{{ slspath }}/files/gateway/yc-api-gateway.logrotate
    - require:
      - yc_pkg: yc-api-gateway
