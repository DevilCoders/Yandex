include:
  - nginx

{% set nginx_configs = ['yc-head.conf'] %}
{%- include 'nginx/install_configs.sls' %}
