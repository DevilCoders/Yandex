/etc/yandex/icecream/backend/config.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://icecream/backend/config.yaml
