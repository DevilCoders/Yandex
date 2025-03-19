include:
    - components.clickhouse.ch-backup.config
    - components.clickhouse.configs.server-config
    - components.clickhouse.configs.server-cluster
    - components.clickhouse.configs.server-users
    - components.clickhouse.configs.reload
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.clickhouse.firewall
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
{% if not salt.pillar.get('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
