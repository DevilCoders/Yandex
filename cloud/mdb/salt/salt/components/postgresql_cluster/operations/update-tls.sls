{% if salt['pillar.get']('data:pg_ssl', True) %}
include:
    - components.postgres.ssl

postgresql-service:
    cmd.run:
        - name: service postgresql reload
        - onchanges:
            - file: /etc/postgresql/ssl
            - file: /etc/postgresql/ssl/server.crt
            - file: /etc/postgresql/ssl/server.key
            - file: /etc/postgresql/ssl/allCAs.pem
        - onlyif:
            - service postgresql status

odyssey:
    cmd.run:
        - name: service odyssey reload
        - onchanges:
            - file: /etc/odyssey/ssl
            - file: /etc/odyssey/ssl/allCAs.pem
            - file: /etc/odyssey/ssl/server.crt
            - file: /etc/odyssey/ssl/server.key
        - onlyif:
            - service odyssey status

{%   endif %}
