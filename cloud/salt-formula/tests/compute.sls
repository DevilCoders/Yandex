yc-compute-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-compute-tests

/etc/yc/compute-tests/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/compute-tests-config.yaml
    - require:
      - yc_pkg: yc-compute-tests

/var/www/html/token:
  file.managed:
    - source: salt://{{ slspath }}/files/token
    - template: jinja
    - makedirs: True
    - user: root
    - group: root

{%- set nginx_configs = ['yc-metadata-mock.conf'] %}
{%- include 'nginx/install_configs.sls' %}
