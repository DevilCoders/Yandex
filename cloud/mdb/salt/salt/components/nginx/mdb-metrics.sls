{% if not salt.pillar.get('data:nginx_no_mdbmetrics', False) %}
{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
{{ deploy_configs('nginx') }}
{% endif %}
