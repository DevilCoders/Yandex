/etc/monrun/salt_hbf/files/:
  file.recurse:
    - source: salt://{{ slspath }}/files/
    - makedirs: True
    - file_mode: 755

/etc/monrun/salt_hbf/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/MANIFEST.json
    - makedirs: True
    - template: jinja

