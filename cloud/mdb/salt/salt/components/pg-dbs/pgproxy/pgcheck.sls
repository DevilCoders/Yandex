/var/log/pgcheck:
    file.directory:
        - user: postgres
        - group: postgres

pgcheck-service:
    service:
        - running
        - enable: true
        - name: mdb-pgcheck
        - require:
            - service: postgresql-service
            - file: /var/log/pgcheck
        - watch:
            - pkg: mdb-pgcheck
            - file: /etc/mdb-pgcheck.yaml
            - file: /etc/init.d/mdb-pgcheck
