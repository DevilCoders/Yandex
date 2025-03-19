/etc/monrun/salt_matilda/matilda_mon.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/matilda_mon.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_matilda/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

