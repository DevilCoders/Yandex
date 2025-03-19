include:
  - nginx

{%- set nginx_configs = ['solomon.conf'] %}
{%- set nginx_folders = {'folder': 'sites-available', 'link': 'sites-enabled'} %}
{%- include 'nginx/install_configs.sls' %}
{%- set nginx_certs = ['cert.pem','key.pem'] %}
{%- include 'nginx/install_certs.sls' %}

solomon-gateway:
    yc_pkg.installed:
    - pkgs:
      - yandex-jdk11
      - yandex-solomon-common
      - yandex-solomon-common-conf: 4287374.stable-2018-12-10
      - yandex-solomon-users
      - yandex-solomon-gateway-conf-cloud
      - yandex-solomon-gateway
      - yandex-solomon-frontend-admin
