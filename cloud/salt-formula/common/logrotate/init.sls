/etc/logrotate.d/rsyslog:
  file.managed:
    - source: salt://{{ slspath }}/files/rsyslog
    - mode: 644
