/etc/monrun/salt_la-per-core/la_per_core.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/la_per_core.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_la-per-core/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

