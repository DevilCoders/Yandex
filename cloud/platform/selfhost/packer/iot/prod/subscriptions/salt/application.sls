Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/subscriptions/config.yaml:
        - source: salt://application/subscriptions.yaml

/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/pod.yaml
    - context:
        subscriptions_application_version: {{ grains['subscriptions_application_version'] }}
