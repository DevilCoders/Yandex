{% set is_walg_enabled = salt.mdb_redis.is_walg_enabled() %}

include:
    - .alias
    - ..common
    - .configs
    - .packages
    - .server
    - .wd_server
    - components.logrotate
{% if salt.pillar.get('data:monrun2', True) %}
    - components.monrun2.redis.server
{% endif %}
{% if salt.pillar.get('data:ship_logs', False) %}
    - components.pushclient2
    - .pushclient
{% endif %}
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
    - .mdb-metrics-install
{% else %}
    - components.mdb-metrics.disable
{% endif %}
    - components.yasmagent
    - .yasmagent
{% if salt.pillar.get('restore-from') %}
{% if salt.mdb_redis.only_walg_enabled() %}
    - .restore-walg
{% endif %}
{% endif %}
{% if salt.pillar.get('data:redis:config:cluster-enabled') == 'yes' %}
{%   if salt.pillar.get('data:monrun2', True) %}
    - components.monrun2.redis.cluster
{%   endif %}
{%   if salt.pillar.get('replica') or salt.pillar.get('new_master') %}
    - .add_cluster_node
{%   endif %}
{% endif %}

    - components.genbackup.disabled

{% if is_walg_enabled %}
    - components.redis.walg
{% elif salt.pillar.get('data:walg:install', False) %}
    - components.redis.walg.pkgs
{% else %}
    - components.redis.walg.disable
{% endif %}
