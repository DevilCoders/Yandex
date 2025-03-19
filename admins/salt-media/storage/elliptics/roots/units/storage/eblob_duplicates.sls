eblob-duplicates:
  file.managed:
    - name: /usr/local/bin/find_duplicates.py
    - user: root
    - source: salt://files/storage/find_duplicates.py
    - group: root
    - mode: 755

  monrun.present:
    - command: /usr/local/bin/find_duplicates.py --check --age 15
    - execution_timeout: 180
    - execution_interval: 3600
    - type: storage

eblob-duplicates-cron:
  file.managed:
    - name: /etc/cron.d/eblob-duplicates
    - contents: >
        0 2 * * 1 root timeout 10800 /usr/local/bin/find_duplicates.py --force >> /var/log/eblob-duplicates.log 2>&1; timeout 10800 /usr/local/bin/find_duplicates.py --recollect >> /var/log/eblob-duplicates.log 2>&1
    - user: root
    - group: root
    - mode: 755

