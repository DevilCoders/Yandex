local-lb.cloud-lab.yandex.net:
  host.present:
    - ip:
      - 127.0.0.1
      - ::1

include:
{#- CLOUD-13828 Workaround for negative DNS response caching #}
  - common.dns
  - nginx

{%- set nginx_configs = ['yc-local-lb.conf'] %}
{%- set nginx_folders = {'folder': 'stream-sites-enabled', 'link': 'stream-sites-available'} %}
{%- include 'nginx/install_configs.sls' %}
