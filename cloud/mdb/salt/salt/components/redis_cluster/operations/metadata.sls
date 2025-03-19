{% set is_walg_enabled = salt.mdb_redis.is_walg_enabled() %}

include:
{% if is_walg_enabled %}
    - components.redis.walg.config
    - components.redis.walg.cron
{% endif %}
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.redis.firewall
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
{% if not salt.pillar.get('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
{% if salt.pillar.get('data:redis:config:cluster-enabled') != 'yes' %}
    - components.redis.sentinel.wd_sentinel
{%     if salt.pillar.get('reset_sentinel', False) %}
    - components.redis.sentinel.reset
{%     endif %}
{% endif %}
