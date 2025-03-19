/etc/monrun/salt_ncq-enable/ncq_enable.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/ncq_enable.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_ncq-enable/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

