/etc/monrun/salt_valve-ping/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_valve-ping/valve-ping.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/valve-ping.sh
    - makedirs: True
    - mode: 755
