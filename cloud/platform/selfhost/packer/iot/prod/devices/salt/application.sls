Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/devices/log4j2.yaml:
        - source: salt://application/log4j2.yaml

/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/pod.yaml
    - context:
        devices_application_version: {{ grains['devices_application_version'] }}

