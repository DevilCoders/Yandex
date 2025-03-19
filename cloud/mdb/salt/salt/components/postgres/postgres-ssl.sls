{% from "components/postgres/pg.jinja" import pg with context %}

/etc/postgresql/ssl:
    file.directory:
        - user: postgres
        - group: postgres
        - makedirs: True
        - mode: 750

/etc/postgresql/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - file: /etc/postgresql/ssl
        - require_in:
            - service: postgresql-service

/etc/postgresql/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - file: /etc/postgresql/ssl
        - require_in:
            - service: postgresql-service


/etc/postgresql/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - file: /etc/postgresql/ssl
        - require_in:
            - service: postgresql-service

{{ pg.prefix }}/.postgresql:
    file.directory:
        - user: postgres
        - group: postgres
        - makedirs: True
        - mode: 755

{{ pg.prefix }}/.postgresql/root.crt:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - file: {{ pg.prefix }}/.postgresql
        - require_in:
            - service: postgresql-service

/home/monitor/.postgresql:
    file.directory:
        - user: monitor
        - group: monitor
        - makedirs: True
        - mode: 755

/home/monitor/.postgresql/root.crt:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: monitor
        - group: monitor
        - mode: 600
        - require:
            - file: /home/monitor/.postgresql
