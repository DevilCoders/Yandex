{% from "map.jinja" import opensearch_version_map with context %}

include:
    - mdb_dbaas_templates.opensearch.base

data:
    opensearch:
        version: {{ opensearch_version_map['2.1'] }}
