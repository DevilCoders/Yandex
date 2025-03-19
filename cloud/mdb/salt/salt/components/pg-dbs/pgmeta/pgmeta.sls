/usr/local/yandex/pgmeta/pgmeta.sql:
    file.managed:
        - source: salt://components/pg-code/pgmeta/pgmeta.sql
        - user: postgres
        - mode: 744
        - makedirs: True
        - require:
            - service: postgresql-service

/usr/local/yandex/pgmeta/s3db.sql:
    file.managed:
        - source: salt://components/pg-code/pgmeta/s3db.sql
        - user: postgres
        - mode: 744
        - makedirs: True
        - require:
            - service: postgresql-service
