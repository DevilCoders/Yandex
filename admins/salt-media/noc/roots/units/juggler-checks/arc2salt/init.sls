/etc/monrun/salt_arc2salt/arc2salt.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/arc2salt.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_arc2salt/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

