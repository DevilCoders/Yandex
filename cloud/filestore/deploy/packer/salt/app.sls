/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://filestore-{{ grains['id'] }}/pod.yaml
    - context:
        app_image: {{ grains['app_image'] }}
        app_version: {{ grains['app_version'] }}
