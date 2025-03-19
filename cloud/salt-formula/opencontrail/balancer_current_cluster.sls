include:
  - nginx

{%- set nginx_configs = ['contrail-discovery.conf', 'contrail-api.conf'] %}
{%- include 'nginx/install_configs.sls' %}
