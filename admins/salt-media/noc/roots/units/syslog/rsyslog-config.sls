/etc/rsyslog.d/60-noc.conf:
  file.managed:
    - source: salt://units/syslog/60-noc.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 666
    - makedirs: True

/usr/local/bin/rsyslog-stats.py:
  file.managed:
    - source: salt://units/syslog/rsyslog-stats.py
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/rsyslog-custom:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: |
        /var/log/rsyslog/*.log
        {
           rotate 3
           daily
           maxsize 1G
           missingok
           notifempty
           delaycompress
           compress
           compresscmd /usr/bin/ionice
           compressoptions -c3 /usr/bin/pzstd -3 -p2
           compressext .zst
           dateext
           dateformat .%Y%m%d-%s
           postrotate
                   invoke-rc.d rsyslog rotate >/dev/null
           endscript
        }

        /var/log/all.log
        {
           rotate 100
           daily
           maxsize 5G
           missingok
           notifempty
           delaycompress
           compress
           compresscmd /usr/bin/ionice
           compressoptions -c3 /usr/bin/pzstd -10 -p2
           compressext .zst
           dateext
           dateformat .%Y%m%d-%s
           postrotate
                   invoke-rc.d rsyslog rotate >/dev/null
           endscript
        }


timeout 55s flock -n /var/tmp/rsyslog-stats.py.lock /usr/local/bin/rsyslog-stats.py /var/log/rsyslog/stats.log:
  cron.present:
    - user: root
