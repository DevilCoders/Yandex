mdb-scripts-dir:
    file.directory:
        - name: 'C:\Program Files\MDB\SQLScripts'
        - user: 'SYSTEM'

mdb-procedures-sql-present:
    file.managed:
        - name: 'C:\Program Files\MDB\SQLScripts\mdb_procedures.sql'
        - source: salt://{{ slspath }}/conf/mdb_procedures.sql
        - template: jinja
        - require:
            - file: mdb-scripts-dir

deploy-mdb-procedures:
    mdb_sqlserver.query_file_run:
        - name: 'C:\\Program Files\\MDB\\SQLScripts\\mdb_procedures.sql'
        - unless: "SELECT 0"
        - require:
            - file: mdb-procedures-sql-present
            - test: sqlserver-config-req
        - onchanges:
            - file: mdb-procedures-sql-present

rollback-filechange-onfail:
    file.absent:
        - name: 'C:\Program Files\MDB\SQLScripts\mdb_procedures.sql'
        - onfail:
            - mdb_sqlserver: deploy-mdb-procedures

ola-maintenance-sql-present:
    file.managed:
        - name: 'C:\Program Files\MDB\SQLScripts\ola_maintenance_solution.sql'
        - source: salt://{{ slspath }}/conf/ola_maintenance_solution.sql
        - template: jinja
        - require:
            - file: mdb-scripts-dir

deploy-ola-maintenance:
    mdb_sqlserver.query_file_run:
        - name: 'C:\\Program Files\\MDB\\SQLScripts\\ola_maintenance_solution.sql'
        - unless: "SELECT 0"
        - require:
            - file: mdb-procedures-sql-present
            - file: ola-maintenance-sql-present
            - test: sqlserver-config-req
        - onchanges:
            - file: ola-maintenance-sql-present

rollback-filechange-onfail-ola-maintenance:
    file.absent:
        - name: 'C:\Program Files\MDB\SQLScripts\ola_maintenance_solution.sql'
        - onfail:
            - mdb_sqlserver: deploy-ola-maintenance
