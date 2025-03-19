/etc/monrun/salt_irrd/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_irrd/irrd_check.py:
  file.managed:
    - source: salt://{{ slspath }}/files/irrd_check.py
    - makedirs: True
    - mode: 755
