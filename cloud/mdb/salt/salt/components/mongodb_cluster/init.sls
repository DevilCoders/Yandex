include:
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.dbaas-compute
    - components.dbaas-compute.network
    - components.firewall
    - components.firewall.external_access_config
    - components.dbaas-compute.apparmor.mongodb
    - components.oslogin
    - .oslogin
{% elif salt.pillar.get('data:dbaas:vtype') == 'porto' %}
    - components.dbaas-porto
    - components.firewall
{% endif %}
    - components.dbaas-billing
    - components.mongodb
