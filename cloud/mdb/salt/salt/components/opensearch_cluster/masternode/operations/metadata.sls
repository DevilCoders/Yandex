include:
    - components.opensearch.config
    - components.opensearch.dashboards-service
{% if not salt['pillar.get']('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.firewall.reload
    - components.firewall.force-reload
    - components.firewall.pillar_configs
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
