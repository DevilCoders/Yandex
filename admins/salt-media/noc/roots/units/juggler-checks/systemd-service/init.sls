/etc/monrun/salt_systemd-service/systemd-service.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/systemd-service.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_systemd-service/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
    - template: jinja
