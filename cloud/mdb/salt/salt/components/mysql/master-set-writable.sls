{% from "components/mysql/map.jinja" import mysql with context %}

set-master-writable:
  mdb_mysql.set_master_writable:
    - connection_default_file: {{ mysql.defaults_file }}
