include:
  - nginx

/Berkanavt/kikimr/ssl/certnew.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub.crt
    - replace: False
    - mode: 0600
    - user: www-data
    - makedirs: True

/Berkanavt/kikimr/ssl/privkey.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub.key
    - replace: False
    - mode: 0600
    - user: www-data
    - makedirs: True

{% set nginx_configs = ['yc-kikimr_proxy.conf'] %}
{%- include 'nginx/install_configs.sls' %}
