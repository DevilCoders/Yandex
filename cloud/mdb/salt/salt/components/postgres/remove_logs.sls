{% from "components/postgres/pg.jinja" import pg with context %}
remove_postgresql_logs:
    cmd.run:
        - name: rm -f {{ pg.log_file_path }}*
        - onlyif: sudo -u postgres psql -c "show log_destination;" | grep csvlog | grep -v stderr && ls {{ pg.log_file_path }}*
        - require:
            - cmd: postgresql-service
