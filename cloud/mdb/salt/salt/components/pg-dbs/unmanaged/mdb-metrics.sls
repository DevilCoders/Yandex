{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}

{{ deploy_configs('pg_unmanaged') }}
{% endif %}
