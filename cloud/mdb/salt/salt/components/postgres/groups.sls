{% from "components/postgres/pg.jinja" import pg with context %}

extend:
    cores-group:
        group:
            - require:
                - pkg: postgresql{{ pg.version.major }}-server
            - members:
                - postgres
