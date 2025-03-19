/etc/monrun/salt_valve-panic/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_valve-panic/valve-panic.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/valve-panic.sh
    - makedirs: True
    - mode: 755
