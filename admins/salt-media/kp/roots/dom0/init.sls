/etc/monitoring/watchdog.conf:
  file.managed:
    - source: salt://{{ slspath }}/watchdog.conf
