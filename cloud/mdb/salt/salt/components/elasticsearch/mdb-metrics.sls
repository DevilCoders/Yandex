{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
include:
    - components.mdb-metrics

{{ deploy_configs('es_common') }}
{% if salt.pillar.get('data:elasticsearch:kibana:enabled', False) %}           
{{ deploy_configs('es_kibana') }}
{% endif %}

{% if salt.pillar.get('data:mdb_metrics:enable_userfault_broken_collector', True) %}
{{ deploy_configs('instance_userfault_broken') }}
{% endif %}
