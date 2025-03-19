{% if salt['grains.get']('pg') and 'role' in salt['grains.get']('pg').keys() and salt['grains.get']('pg')['role'] == 'master' %}
{% set db_code_map = salt['pillar.get']('data:db_code_map', {}) %}

{% for dbname in salt['pillar.get']('data:config:databases', []) %}

{% if dbname in db_code_map.keys() %}
{% set sql = db_code_map[dbname] %}
{% else %}
{% set sql = 'pgmeta.sql' %}
{% endif %}

create_db_{{ dbname }}:
    postgres_database.present:
        - name: {{ dbname }}
        - user: postgres
        - require:
            - cmd: postgresql-service

apply_code_{{ dbname }}:
    postgresql_cmd.psql_file:
        - name: /usr/local/yandex/pgmeta/{{ sql }}
        - maintenance_db: {{ dbname }}
        - onchanges:
            - file: /usr/local/yandex/pgmeta/{{ sql }}
            - postgres_database: create_db_{{ dbname }}
{% endfor %}
{% endif %}
