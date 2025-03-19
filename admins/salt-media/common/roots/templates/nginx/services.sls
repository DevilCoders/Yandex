{% from slspath + "/map.jinja" import nginx with context %}

install_nginx:
  pkg.installed:
    - name: nginx
{%- if nginx.nginx_version is defined %}
    - version: {{ nginx.nginx_version }}
{%- endif %}

nginx_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in nginx.packages %}
      - {{ pkg }}
    {%- endfor %}

nginx-service-reconfigure:
  service.running:
    - name: {{ nginx.service }}
    - enable: True
    {% if nginx.only_reload %}
    - reload: True
    {% endif %}
    - require:
      - module: nginx-config-test
    - watch:
      - file: /etc/nginx/*

# test nginx config
nginx-config-test:
  module.wait:
    - name: nginx.configtest
