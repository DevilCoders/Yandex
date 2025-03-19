/etc/monrun/salt_META/meta.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/meta.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_META/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
