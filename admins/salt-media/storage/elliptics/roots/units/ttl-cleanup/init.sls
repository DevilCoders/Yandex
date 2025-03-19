/etc/cron.d/ttl-cleanup:
  file.absent

# /etc/cron.d/ttl-cleanup:
#   file.managed:
#    - source: salt://files/ttl-cleanup/etc/cron.d/ttl-cleanup
#    - user: root
#    - group: root
#    - mode: 644

/usr/bin/ttl-cleanup.py:
  yafile.managed:
   - source: salt://files/ttl-cleanup/usr/bin/ttl-cleanup.py
   - user: root
   - group: root
   - mode: 755
   - template: jinja

mail-tmp-cleanup:
  monrun.present:
    - command: "if [ -e /var/tmp/ttl-cleanup ]; then head -1 /var/tmp/ttl-cleanup; else echo '2; monitor file unavailable'; fi"
    - execution_interval: 600
    - execution_timeout: 180
