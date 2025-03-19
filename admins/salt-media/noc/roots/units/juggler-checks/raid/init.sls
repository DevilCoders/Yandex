/etc/monrun/salt_raid/raid.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/raid.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_raid/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

