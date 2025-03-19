/etc/monrun/salt_unispace/unispace.pl:
  file.managed:
    - source: salt://{{ slspath }}/files/unispace.pl
    - makedirs: True
    - mode: 755

/etc/monrun/salt_unispace/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

