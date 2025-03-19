include:
  - compute-node-vpc.underlay
  - common.network_interfaces.base_ipv4
  - common.push-client
  - snapshot
  - graphics
  - identity
  - common.hbf-agent
  - cli

{%- set nginx_configs = ['yc-images.conf'] %}
{%- include 'nginx/install_configs.sls' %}
