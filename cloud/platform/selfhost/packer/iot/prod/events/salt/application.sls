Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/events_broker.yaml:
        - source: salt://application/events_broker.yaml

/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/pod.yaml
    - context:
        events_application_version: {{ grains['events_application_version'] }}
