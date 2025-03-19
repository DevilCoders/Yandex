adding_landing_page:
    file.managed:
      - name: /usr/share/nginx/html/index.html
      - source: salt://{{ slspath }}/plain_files/rootfs/usr/share/nginx/html/index.html
      - makedirs: True

include:
  - nginx
{%- set nginx_configs = ['rkn_landing_page.conf'] %}
{%- include 'nginx/install_configs.sls' %}