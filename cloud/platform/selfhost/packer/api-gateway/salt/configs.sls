config_files:
  file.managed:
    - user: root
    - group: root
    - mode: '0644'
    - makedirs: True
    - names:
      - /etc/api/configs/als/als.yaml:
        - source: salt://files/als.yaml
      - /etc/api/configs/envoy/envoy.yaml:
        - source: salt://files/envoy.yaml
      - /etc/api/configs/gateway/gateway.yaml:
        - source: salt://files/gateway.yaml
      - /etc/api/configs/configserver/configserver.yaml:
        - source: salt://files/configserver.yaml
      - /etc/api/configs/configserver/envoy-resources.yaml:
        - source: salt://files/envoy-resources.yaml
      - /etc/api/configs/configserver/gateway-services.yaml:
        - source: salt://files/gateway-services.yaml
