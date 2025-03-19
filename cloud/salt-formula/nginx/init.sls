{% set hostname = grains['nodename'] %}
{% set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}
{% if base_role == 's3-proxy' %}
  {% set nginx_conf_source = slspath | path_join('../s3-proxy/files/nginx.conf') %}
{% else %}
  {% set nginx_conf_source = slspath | path_join('/files/nginx.conf') %}
{% endif %}

nginx_package:
#TODO: Temporary hardcode version
  yc_pkg.installed:
    - name: nginx
    - pkgs:
      - nginx

nginx_service:
  service.running:
    - name: nginx
    - enable: True
    - require:
      - file: /etc/nginx/nginx.conf
    - watch:
      - yc_pkg: nginx

{# NOTE: nginx must be restarted when new version is installed, but only reloaded when configs change. CLOUD-20322 #}
nginx_reload_on_config_change:
  service.running:
    - name: nginx
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/stream-sites-enabled
      - file: /etc/nginx/nginx.conf
      - file: /etc/nginx/conf.d/logformat.conf
      - file: /etc/nginx/proxy_params
      - file: /etc/nginx/sites-enabled/status.conf

/etc/nginx/stream-sites-enabled:
  file.directory:
    - require:
      - yc_pkg: nginx

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{ nginx_conf_source }}
    - template: jinja
    - defaults:
        base_role: {{ base_role }}
    - require:
      - yc_pkg: nginx

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate.conf
    - require:
      - yc_pkg: nginx

/etc/nginx/conf.d/logformat.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/logformat.conf
    - require:
      - yc_pkg: nginx

/etc/nginx/proxy_params:
  file.managed:
    - source: salt://{{ slspath }}/files/proxy_params
    - require:
      - yc_pkg: nginx

/etc/nginx/sites-enabled/status.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/status.conf
    - require:
      - yc_pkg: nginx

{%- set nginx_certs = [ pillar['secrets']['api']['cert'], pillar['secrets']['api']['key'] ] %}
{%- include 'nginx/install_certs.sls' %}
