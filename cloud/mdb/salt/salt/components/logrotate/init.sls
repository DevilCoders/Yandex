/etc/logrotate.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644

/etc/cron.yandex/logrotate.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/logrotate.sh
        - mode: 755
        - require:
            - file: /etc/logrotate.conf

/usr/local/yandex/log_curator.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/log_curator.py
        - mode: 755
        - makedirs: True

/etc/cron.daily/logrotate:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/logrotate.daily
        - mode: 755
        - require:
            - file: /etc/cron.yandex/logrotate.sh

/etc/cron.d/logrotate_by_size:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/logrotate_by_size.cron
        - mode: 644
        - require:
            - file: /etc/cron.yandex/logrotate.sh

/etc/cron.d/trim_cores:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/trim_cores.cron
        - mode: 644
        - require:
            - file: /usr/local/yandex/log_curator.py

/etc/logrotate.d/salt-common:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/salt-common
        - mode: 644

