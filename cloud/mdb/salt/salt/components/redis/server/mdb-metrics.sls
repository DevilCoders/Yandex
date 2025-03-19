{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
{{ deploy_configs('redis_common') }}
