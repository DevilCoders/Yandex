{% from "components/mysql/map.jinja" import mysql with context %}

/etc/mysql/ssl:
  file.directory:
   - user: mysql
   - group: mysql
   - makedirs: True
   - mode: 750
   - require:
      - user: mysql-user
   - require_in:
        - test: mysql-service-req

/etc/mysql/ssl/server.key:
  file.managed:
      - contents_pillar: cert.key
      - template: jinja
      - user: mysql
      - group: mysql
      - mode: 600
      - require:
          - file: /etc/mysql/ssl
      - require_in:
          - test: mysql-service-req

/etc/mysql/ssl/server.crt:
  file.managed:
      - contents_pillar: cert.crt
      - template: jinja
      - user: mysql
      - group: mysql
      - mode: 600
      - require:
          - file: /etc/mysql/ssl
      - require_in:
          - test: mysql-service-req

/etc/mysql/ssl/allCAs.pem:
  file.managed:
      - contents_pillar: cert.ca
      - template: jinja
      - user: mysql
      - group: mysql
      - mode: 600
      - require:
          - file: /etc/mysql/ssl
      - require_in:
          - test: mysql-service-req

{% if salt['pillar.get']('data:versions:mysql:minor_version') != '5.7.25' %}
do-reload_ssl:
  mysql_query.run:
    - database: mysql
    - query:    "ALTER INSTANCE RELOAD TLS"
    - connection_default_file: {{ mysql.defaults_file }}
    - require:
        - file: {{ mysql.defaults_file }}
        - test: mysql-service-ready
    - onchanges:
        - file: /etc/mysql/ssl/server.key
        - file: /etc/mysql/ssl/server.crt
        - file: /etc/mysql/ssl/allCAs.pem
{% endif %}
