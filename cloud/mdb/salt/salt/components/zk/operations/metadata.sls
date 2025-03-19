{% set dbaas = salt.pillar.get('data:dbaas', {}) %}

{% if salt.pillar.get('data:dbaas:vtype') == 'compute' or not dbaas.get('cloud') or not salt.pillar.get('skip-billing', False) %}
include:
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
{% if not dbaas.get('cloud') %}
    - components.zk.firewall
    - components.firewall.reload
{% endif %}
{% if not salt.pillar.get('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
{% endif %}
