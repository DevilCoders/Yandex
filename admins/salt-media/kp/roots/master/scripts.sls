/usr/local/sbin/kp-check-cron.pl:
  file.managed:
    - source: salt://{{slspath}}/files/usr/local/sbin/kp-check-cron.pl
    - mode: 0700
    - user: root
    - group: root
