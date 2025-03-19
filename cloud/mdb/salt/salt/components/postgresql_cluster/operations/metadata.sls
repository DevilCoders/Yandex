include:
{% if salt['pillar.get']('data:use_walg', True) %}
    - components.postgres.walg-cron
    - components.postgres.walg-config
{% endif %}
    - components.postgres.sync-replication-slots
    - components.postgres.pgsync
{% if not salt['pillar.get']('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
    - components.postgres.firewall
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
