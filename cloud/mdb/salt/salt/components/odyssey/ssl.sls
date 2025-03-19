{% from "components/postgres/pg.jinja" import pg with context %}

/etc/{{ pg.connection_pooler }}/ssl:
    file.directory:
        - user: {{ pg.bouncer_user }}
        - group: {{ pg.bouncer_user }}
        - makedirs: True
        - mode: 750
        - require_in:
            - service: {{ pg.connection_pooler }}

/etc/{{ pg.connection_pooler }}/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: {{ pg.bouncer_user }}
        - group: {{ pg.bouncer_user }}
        - mode: 600
        - require:
            - file: /etc/{{ pg.connection_pooler }}/ssl
        - require_in:
            - service: {{ pg.connection_pooler }}

/etc/{{ pg.connection_pooler }}/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: {{ pg.bouncer_user }}
        - group: {{ pg.bouncer_user }}
        - mode: 600
        - require:
            - file: /etc/{{ pg.connection_pooler }}/ssl
        - require_in:
            - service: {{ pg.connection_pooler }}

/etc/{{ pg.connection_pooler }}/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: {{ pg.bouncer_user }}
        - group: {{ pg.bouncer_user }}
        - mode: 600
        - require:
            - file: /etc/{{ pg.connection_pooler }}/ssl
        - require_in:
            - service: {{ pg.connection_pooler }}
