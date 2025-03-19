/etc/monrun/salt_decapinger/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_decapinger/process.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/process.sh
    - makedirs: True
    - mode: 755
