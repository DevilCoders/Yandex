/etc/monrun/salt_bmp/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_bmp/bmp_check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/bmp_check.sh
    - makedirs: True
    - mode: 755
