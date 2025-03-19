do-flush-hosts:
  mysql_query.run:
    - database: mysql
    - query:    "FLUSH HOSTS"
    - connection_default_file: /home/mysql/.my.cnf
