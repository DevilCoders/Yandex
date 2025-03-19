/etc/monrun/salt_master/salt-master.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/salt-master.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_master/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

