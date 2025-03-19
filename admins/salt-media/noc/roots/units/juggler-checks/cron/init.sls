/etc/monrun/salt_cron/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_cron/cron.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cron.sh
    - makedirs: True
    - mode: 755
