{% from "components/sqlserver/map.jinja" import sqlserver with context %}

{% set name = salt['pillar.get']('target-database') %}


{% if not sqlserver.is_replica %}

db-not-in-ag-{{ name|yaml_encode }}:
    mdb_sqlserver.db_not_in_ag:
        - name: {{ name|yaml_encode }}
{% if sqlserver.edition == 'enterprise' %}
        - ag_name: AG1
{% endif %}
{% if sqlserver.edition == 'standard' %}
        - ag_name: {{ name|yaml_encode }}
{% endif %}
        - require_in:
            - mdb_sqlserver: db-absent-{{ name|yaml_encode }}

{%     if sqlserver.edition == 'standard' %}
ag-absent-{{ name|yaml_encode }}:
    mdb_sqlserver.ag_absent:
        - name: {{ name|yaml_encode }}
        - require:
            - mdb_sqlserver: db-not-in-ag-{{ name|yaml_encode }}
{%     endif %}

{% else %}

db-not-in-ag-{{ name|yaml_encode }}:
    mdb_sqlserver.secondary_db_not_in_ag:
        - name: {{ name|yaml_encode }}
        - require_in:
            - mdb_sqlserver: db-absent-{{ name|yaml_encode }}


{% endif %}

db-absent-{{ name|yaml_encode }}:
    mdb_sqlserver.db_absent:
        - name: {{ name|yaml_encode }}
