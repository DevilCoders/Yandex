{% from "components/mysql/map.jinja" import mysql with context %}

mysql-update-timezones:
    cmd.run:
        - name: >
            mysql_tzinfo_to_sql /usr/share/zoneinfo | mysql --defaults-file={{ mysql.defaults_file }} mysql
        - require:
            - pkg: tzdata
            - test: mysql-service-ready
            - file: {{ mysql.defaults_file }}
        - onchanges:
            - pkg: tzdata
