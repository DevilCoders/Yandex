/etc/gandalf/gandalf.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/gandalf/gandalf.yaml
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - template: jinja

/var/log/gandalf:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

gandalf-logrotate:
  file.managed:
    - name: /etc/logrotate.d/gandalf
    - contents: |
        /var/log/gandalf/*.log
        {
            daily
            maxsize 2048M
            missingok
            rotate 14
            compress
            compresscmd /usr/bin/pzstd
            compressoptions -8 -p1
            compressext .zst
            notifempty
            dateext
            dateformat .%Y%m%d-%s
            postrotate
                kill -HUP `pidof gandalf`
            endscript
        }

gandalf:
  monrun.present:
    - command: /usr/local/bin/gandalf_monrun.py
    - execution_interval: 300
    - execution_timeout: 60
    - type: mastermind

  file.managed:
    - name: /usr/local/bin/gandalf_monrun.py
    - source: salt://{{ slspath }}/files/usr/local/bin/gandalf_monrun.py
    - user: root
    - group: root
    - mode: 755
