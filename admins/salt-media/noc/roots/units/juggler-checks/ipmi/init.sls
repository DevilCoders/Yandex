/etc/monrun/salt_ipmi/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_ipmi/ipmi.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/ipmi.sh
    - makedirs: True
    - mode: 755
