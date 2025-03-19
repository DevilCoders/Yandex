/etc/cron.d/initial_salt_minion_restart:
  file.absent

/var/spool/cron/crontabs/root:
  file.managed:
    - replace: False
