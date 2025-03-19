{% set oozie = salt['pillar.get']('data:settings:oozie') %}

oozie-postgres-database:
    postgres_database.present:
        - name: {{ oozie['db_name'] }}
        - require:
            - service: service-postgresql

oozie-postgres-user:
    postgres_user.present:
        - name: {{ oozie['db_user'] }}
        - encrypted: True
        - login: True
        - password: {{ oozie['db_password'] }}
        - require:
            - postgres_database: oozie-postgres-database
            - service: service-postgresql

oozie-postgres-privileges:
    postgres_privileges.present:
        - name: {{ oozie['db_user'] }}
        - object_name: {{ oozie['db_name'] }}
        - object_type: database
        - privileges:
            - ALL
        - require:
            - service: service-postgresql
            - postgres_database: oozie-postgres-database
            - postgres_user: oozie-postgres-user

