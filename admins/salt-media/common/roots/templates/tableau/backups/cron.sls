/etc/cron.d/tableau-backup:
  file.managed:
    - source: salt://{{ slspath }}/files/tableau_backup.cron
    - user: root
    - group: root
    - mode: 0644
