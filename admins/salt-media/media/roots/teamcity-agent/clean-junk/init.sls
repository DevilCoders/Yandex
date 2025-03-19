/usr/local/sbin/clean_junk.sh:
  file.managed:
    - source: salt://{{ slspath }}/clean_junk.sh
    - mode: 755

/etc/cron.d/clean_junk:
  file.managed:
    - contents: |
        # CADMIN-8739, KPDUTY-2283
        0 4 * * * root /usr/local/sbin/clean_junk.sh
