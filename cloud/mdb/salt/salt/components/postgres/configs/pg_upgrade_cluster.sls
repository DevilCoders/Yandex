/usr/local/yandex/pg_upgrade_cluster.py:
    file.managed:
        - source: salt://components/postgres/conf/pg_upgrade_cluster.py
        - user: postgres
        - group: postgres
        - mode: 755
        - makedirs: True
