{% set dbaas = salt.pillar.get('data:dbaas', {}) %}
{% set working_with_kafka = salt.pillar.get('data:zk:apparmor_disabled', False) %}

include:
   - .main
   - .systemd
   - components.logrotate
   - components.monrun2.zookeeper
{% if dbaas %}
   - .resize
{% endif %}
{% if dbaas.get('vtype') == 'compute' %}
   - .firewall
   - .compute
   - components.firewall
   - components.firewall.external_access_config
   - components.dbaas-compute
   - components.dbaas-compute.apparmor.zookeeper
{%     if not working_with_kafka %}
   - components.oslogin
   - .oslogin
{%     endif %}
{%     if dbaas.get('cluster_id') %}
   - components.dbaas-compute.network
{%     else %}
   - components.dbaas-compute-controlplane.network
{%     endif %}
{% elif dbaas.get('vtype') == 'porto' %}
   - components.dbaas-porto
{% endif %}
{% if not dbaas.get('cloud') %}
   - components.firewall
   - components.firewall.external_access_config
{% endif %}
{% if dbaas.get('cluster_id') and not working_with_kafka %}
   - components.pushclient2
   - .pushclient
   - components.dbaas-billing
{% endif %}
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
   - .mdb-metrics
   - components.yasmagent
   - .yasmagent
{% else %}
   - components.mdb-metrics.disable
{% endif %}
{% if salt.pillar.get('data:unmanaged:enable_zk_tls', False) %}
   - .zkpass
{% endif %}
{% if salt.pillar.get('service-restart', False) %}
   - .restart
{% endif %}
