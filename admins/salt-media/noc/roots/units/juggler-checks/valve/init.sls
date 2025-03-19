/etc/monrun/salt_valve/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_valve/valve.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/valve.sh
    - makedirs: True
    - mode: 755
