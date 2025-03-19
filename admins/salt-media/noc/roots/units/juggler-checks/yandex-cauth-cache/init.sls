/etc/monrun/salt_yandex-cauth-cache/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

