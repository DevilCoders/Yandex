Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/mqtt-server.yaml:
        - source: salt://application/mqtt-server.yaml

/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/pod.yaml
    - context:
        mqtt_application_version: {{ grains['mqtt_application_version'] }}
