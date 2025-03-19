/etc/monrun/salt_watchdog/watchdog.pl:
  file.managed:
    - source: salt://{{ slspath }}/files/watchdog.pl
    - makedirs: True
    - mode: 755

/etc/monrun/salt_watchdog/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

