{% set settings = salt['pillar.get']('data:settings') %}

hive-postgres-database:
    postgres_database.present:
        - name: {{ settings['hive_db_name'] }}
        - require:
            - service: service-postgresql

hive-postgres-user:
    postgres_user.present:
        - name: {{ settings['hive_db_user'] }}
        - encrypted: True
        - login: True
        - password: {{ settings['hive_db_password'] }}
        - require:
            - postgres_database: hive-postgres-database
            - service: service-postgresql

hive-postgres-privileges:
    postgres_privileges.present:
        - name: {{ settings['hive_db_user'] }}
        - object_name: {{ settings['hive_db_name'] }}
        - object_type: database
        - privileges: 
            - ALL
        - require:
            - service: service-postgresql
            - postgres_database: hive-postgres-database
            - postgres_user: hive-postgres-user

