{% from "components/postgres/pg.jinja" import pg with context %}

include:
    - components.postgres.pkgs_ubuntu

{% if salt['pillar.get']('data:use_postgis', False) %}
{% if pg.version.major_num == 1300 %}
old-pgrouting-package:
    pkg.installed:
        - pkgs:
            - postgresql-12-pgrouting: '3.0.2-1.pgdg18.04+1'
            - postgresql-12-pgrouting-scripts: '3.0.2-1.pgdg18.04+1'
        - require:
            - pkgrepo: pgdg
        - require_in:
            - cmd: all-pg-packages-are-ready
{% elif pg.version.major_num == 1400 %}
old-pgrouting-package:
    pkg.installed:
        - pkgs:
            - postgresql-13-pgrouting: '3.3.1-1'
            - postgresql-13-pgrouting-scripts: '3.3.1-1'
        - require:
            - pkgrepo: pgdg
        - require_in:
            - cmd: all-pg-packages-are-ready
{% endif %}
{% endif %}

{% if salt['pillar.get']('data:dbaas_metadb') %}
{# For upgrading MetaDB, see pg-dbs/dbaas_metadb/init.sls #}
{% from "components/postgres/pg.jinja" import pg with context %}
postgresql-plpython3-{{ pg.version.major }}:
    pkg.installed:
        - version: {{ pg.version.pkg }}
{% endif %}
