{% set hostname = grains['nodename'] %}
{% set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] %}
include:
{# Do not deploy head role on single-node CVM as it is really compute networking-wise #}
  - cli
  - common.network_interfaces.base_ipv4
  - common.hbf-agent
  - common.push-client
  - compute
  - compute-head
  - e2e-tests
  - e2e-tests.head
  - opencontrail.balancer_all_clusters
  - roles.local_proxy
  - roles.scheduler
  - snapshot.balancer_az

{% if 'tests' not in host_roles %}
{%- set nginx_configs = ['yc-images.conf'] %}
{%- include 'nginx/install_configs.sls' %}
{% endif %}
