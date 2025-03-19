/etc/monitoring/watchdog.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/watchdog.conf
