{%- set hostname = grains['nodename'] %}
{#- balancer and snapshot service listen on the same port, so skip #}
{%- if 'snapshot' not in salt['grains.get']('cluster_map:hosts:%s:roles' % hostname, []) %}
include:
  - nginx

{%- set nginx_configs = ['snapshot-grpc.conf'] %}
{%- set nginx_folders = {'folder': 'stream-sites-available', 'link': 'stream-sites-enabled'} %}
{%- include 'nginx/install_configs.sls' %}
{%- endif %}
