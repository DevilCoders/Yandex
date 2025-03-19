/etc/monrun/salt_autov6here/autov6.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/autov6.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_autov6here/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

