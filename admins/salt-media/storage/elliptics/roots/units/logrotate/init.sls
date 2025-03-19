/etc/cron.d/logrotate-locked:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/logrotate-locked
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja


/etc/cron.d/logrotate-hourly:
  file.absent:
    - name: "/etc/cron.d/logrotate-hourly"
