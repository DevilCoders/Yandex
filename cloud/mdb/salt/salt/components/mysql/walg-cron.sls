/etc/cron.d/mysql_walg:
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/mysql_walg
        - template: jinja
        - mode: 644
