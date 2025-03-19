{% set unit = 'mds-logbackup' %}

mds-logbackup-pkg:
  pkg.installed:
    - pkgs:
      - mds-logbackup

/etc/mds-logbackup/config-mds-logbackup.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/mds-logbackup/config-mds-logbackup.json
    - makedirs: True
    - template: jinja

/etc/cron.d/mds-logbackup-new:
  file.managed:
    - contents: |
        3 0 * * * root bash -c 'sleep $(( $RANDOM \% 18000 ))' && flock -n /tmp/mds-logbackup-new.lock  -c "/usr/bin/mds-logbackup --config /etc/mds-logbackup/config-mds-logbackup.json >/dev/null 2>&1"
        3 6,8,10,12,14,16,18,20,22 * * * root bash -c 'sleep $(( $RANDOM \% 7200 ))' && flock -n /tmp/mds-logbackup-new.lock  -c "/usr/bin/mds-logbackup --config /etc/mds-logbackup/config-mds-logbackup.json >/dev/null 2>&1"
    - user: root
    - group: root
    - mode: 755

# MDS-15640
/etc/cron.d/mds-logbackup:
  file.absent

/var/log/mds-logbackup:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

mds-logbackup-arc-logrotate:
  file.managed:
    - name: /etc/logrotate.d/mds-logbackup-arc
    - contents: |
        /var/log/mds-logbackup/*.log
        {
            weekly
            maxsize 4G
            missingok
            rotate 35
            compress
            compresscmd /usr/bin/pzstd
            compressoptions -8 -p1
            compressext .zst
            notifempty
            dateext
            dateformat .%Y%m%d-%s
        }

mds-logbackup-arc:
  monrun.present:
    - command: "if [ -e /var/tmp/mds-logbackup.log ]; then head -1 /var/tmp/mds-logbackup.log; else echo '1; monitor file unavailable'; fi"
    - execution_interval: 60
    - execution_timeout: 30

/usr/bin/dnet-recovery-compress.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/dnet-recovery-compress.sh
    - user: root
    - group: root
    - mode: 755

/usr/bin/mastermind-jobs.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/mastermind-jobs.sh
    - user: root
    - group: root
    - mode: 755

/usr/bin/karl-jobs.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/karl-jobs.sh
    - user: root
    - group: root
    - mode: 755

/etc/cron.d/dnet-recovery-compress:
  file.managed:
    - contents: |
        13 3 * * * root /usr/bin/dnet-recovery-compress.sh >/dev/null 2>&1
    - user: root
    - group: root
    - mode: 755

/etc/cron.d/mastermind-jobs:
  file.managed:
    - contents: |
        47 */4 * * * root /bin/bash /usr/bin/mastermind-jobs.sh >/dev/null 2>&1
    - user: root
    - group: root
    - mode: 755

/etc/cron.d/karl-jobs:
  file.managed:
    - contents: |
        47 */4 * * * root /bin/bash /usr/bin/karl-jobs.sh >/dev/null 2>&1
    - user: root
    - group: root
    - mode: 755
