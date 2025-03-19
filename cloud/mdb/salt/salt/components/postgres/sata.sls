{% from "components/postgres/pg.jinja" import pg with context %}

sata-tablespace:
    postgres_tablespace.present:
        - name: sata
        - directory: {{ pg.prefix }}/slow
        - options:
            seq_page_cost: 1.0
            random_page_cost: 4.0
        - require:
            - service:
                postgresql-service
