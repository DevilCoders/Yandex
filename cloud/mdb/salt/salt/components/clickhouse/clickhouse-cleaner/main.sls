/usr/local/yandex/clickhouse-cleaner.py:
    file.managed:
        - source: salt://{{ slspath }}/clickhouse-cleaner.py
        - mode: 755
        - require:
            - pkg: clickhouse-packages

/etc/logrotate.d/clickhouse-cleaner:
    file.managed:
        - source: salt://{{ slspath }}/logrotate.conf
        - mode: 644
        - require:
            - file: /usr/local/yandex/clickhouse-cleaner.py
        - require_in:
            - file: /etc/cron.d/clickhouse-cleaner

ch-cleaner-logs-dir:
    file.directory:
        - name: /var/log/yandex
        - dir_mode: 775
        - require:
            - file: /usr/local/yandex/clickhouse-cleaner.py
        - require_in:
            - file: /etc/cron.d/clickhouse-cleaner

/etc/cron.d/clickhouse-cleaner:
    file.managed:
        - source: salt://{{ slspath }}/clickhouse-cleaner.cron
        - mode: 644
        - require:
            - fs: ch-cleaner-config
