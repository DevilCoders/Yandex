{% from "components/mysql/map.jinja" import mysql with context %}
include:
    - components.mysql.databases
    - components.mysql.users-grants

extend:
    mysql-databases-ready:
        test.nop:
            - require_in:
                - mdb_mysql: mysql-ensure-grants
