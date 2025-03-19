/etc/rc.conf.local:
  file.managed:
    - source: salt://{{ slspath }}/etc/rc.conf.local
