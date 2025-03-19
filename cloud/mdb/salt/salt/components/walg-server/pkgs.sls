{% set versions = ['10', '11', '12'] %}
{% set extensions = [
        'heapcheck'
    ]
%}

include:
    - components.repositories.apt.pgdg

postgres-packages:
    pkg.latest:
        - pkgs:
            - wal-g
            - python3-psycopg2
            - postgresql-common
            - postgresql-client-common
            - pgdg-keyring
            - libpq5
{% for version in versions %}
            - postgresql-{{ version }}
            - postgresql-client-{{ version }}
{% for extension in extensions %}
            - postgresql-{{ version }}-{{ extension }}
{% endfor %}
{% endfor %}
        - fromrepo: stable
        - prereq_in:
            - cmd: repositories-ready

{% for version in versions %}
postgresql@{{ version }}-main:
    service.dead:
        - enable: False
        - require:
            - pkg: postgres-packages

/etc/postgresql/{{ version }}/main:
    file.absent:
        - require:
            - service: postgresql@{{ version }}-main

/var/lib/postgresql/{{ version }}/main:
    file.absent:
        - require:
            - service: postgresql@{{ version }}-main
{% endfor %}
