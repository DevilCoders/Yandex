include:
  - common.hbf-agent.compute
  - common.netmon
  - common.push-client
  - compute-metadata
  - compute-node
  - e2e-tests
  - iam-agent
  - nginx
  - osquery
  - serialproxy

{% set nginx_configs = ['yc-compute.conf'] %}
{%- include 'nginx/install_configs.sls' %}
