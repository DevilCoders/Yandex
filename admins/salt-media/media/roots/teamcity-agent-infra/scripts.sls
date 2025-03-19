/usr/local/bin/alet-yav-getter.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/alet-yav-getter.sh
    - user: root
    - group: root
    - mode: 0755

/usr/local/sbin/clean_junk.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/sbin/clean_junk.sh
    - user: root
    - group: root
    - mode: 0755

/etc/cron.d/clean_junk:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents: |
        # CADMIN-8739, KPDUTY-2283
        0 4 * * * root /usr/local/sbin/clean_junk.sh
