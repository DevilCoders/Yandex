{% from "components/mysql/map.jinja" import mysql with context %}
include:
    - components.mysql.service
    - components.mysql.users
    - components.mysql.users-grants

extend:
    mysql-users-ready:
        test.nop:
            - require_in:
                - mdb_mysql: mysql-ensure-grants
