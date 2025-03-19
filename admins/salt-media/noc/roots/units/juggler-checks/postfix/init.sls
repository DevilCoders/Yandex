/etc/monrun/salt_postfix/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_postfix/postfix.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/postfix.sh
    - makedirs: True
    - mode: 755
