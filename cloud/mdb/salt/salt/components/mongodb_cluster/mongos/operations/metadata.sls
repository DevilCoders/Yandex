include:
    - components.common.dbaas
    - components.common.clean-etc-hosts
    - components.common.etc-hosts
{% if not salt.pillar.get('include-metadata') %}
    - components.mongodb.mongos-config
{% endif %}
{% if salt.pillar.get('mongodb-delete-shard-name') %}
    - components.mongodb.mongos-delete-shard
{% endif %}
{% if salt.pillar.get('mongodb-add-shards') %}
    - components.mongodb.mongos-add-shards
{% endif %}
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.firewall.reload
    - components.firewall.force-reload
    - components.firewall.pillar_configs
    - components.firewall.user_net_config
    - components.firewall.external_access_config
    - components.mongodb.firewall
{% endif %}
{% if not salt.pillar.get('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
