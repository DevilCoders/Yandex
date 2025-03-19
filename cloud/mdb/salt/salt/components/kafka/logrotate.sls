/etc/logrotate.d/rsyslog:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/syslog
        - makedirs: True
        - mode: 644
