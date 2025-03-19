include:
    - components.common.dbaas
    - components.common.clean-etc-hosts
    - components.common.etc-hosts
    - components.mongodb.mongocfg-remove-deleted-rs-members
    - components.mongodb.walg.cron
    - components.mongodb.walg.config
    - components.mongodb.tools.configs
{% if not salt.pillar.get('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
{% if not salt.pillar.get('include-metadata') %}
    - components.mongodb.mongocfg-config
{% endif %}
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.firewall.reload
    - components.firewall.force-reload
    - components.firewall.pillar_configs
    - components.firewall.user_net_config
    - components.firewall.external_access_config
    - components.mongodb.firewall
{% endif %}
