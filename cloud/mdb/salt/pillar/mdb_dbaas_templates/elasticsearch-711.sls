{% from "map.jinja" import es_version_map with context %}

include:
    - mdb_dbaas_templates.elasticsearch_base

data:
    elasticsearch:
        version: {{ es_version_map['7.11'] }}
