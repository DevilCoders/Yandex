python-psycopg2:
    pkg.installed:
        - version: '2.8.3-2~pgdg18.04+1+yandex0'
        - prereq_in:
            - cmd: repositories-ready

python3-psycopg2:
    pkg.installed:
        - version: '2.7.4-1'
        - prereq_in:
            - cmd: repositories-ready

pigz:
    pkg.installed:
        - prereq_in:
            - cmd: repositories-ready

shelf-tool:
{% if salt['grains.get']('virtual') == 'physical' %}
    pkg.installed:
        - version: '21-46e0011'
        - prereq_in:
            - cmd: repositories-ready
{% else %}
    pkg.purged
{% endif %}

pgmigrate-pkg:
    pkg.installed:
        - prereq_in:
            - cmd: repositories-ready
        - pkgs:
            - mdb-pgmigrate: '1.8502211'

purge-old-pgmigrate-pkgs:
    pkg.purged:
        - pkgs:
            - yamail-pgmigrate
            - python-future
            - python-sqlparse
        - require:
            - pkg: pgmigrate-pkg
