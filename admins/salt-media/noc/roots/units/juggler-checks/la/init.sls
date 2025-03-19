/etc/monrun/salt_la/la.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/la.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_la/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

