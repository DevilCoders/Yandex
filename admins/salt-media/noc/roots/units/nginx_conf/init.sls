nginx:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/*

{% if pillar['nginx_conf_solomon_enabled']|default(true) %}
include:
  - units.solomon
{% endif %}

{% if pillar['nginx_conf']|default(true) %}
/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.conf
{% endif %}

{% if pillar['nginx_conf_tskv_enabled']|default(true) %}
/etc/nginx/conf.d/00-tskv.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/conf.d/00-tskv.conf
    - makedirs: True
{% else %}
/etc/nginx/conf.d/00-tskv.conf:
  file.absent
{% endif %}

/etc/nginx/conf.d/90-lua.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/conf.d/90-lua.conf
    - makedirs: True

/etc/nginx/lua/init.lua:
  file.managed:
    - source: salt://{{ slspath }}/files/lua/init.lua
    - makedirs: True

/etc/nginx/lua.init.d:
  file.directory:
    - makedirs: True

{% if pillar['nginx_conf_solomon_enabled']|default(true) %}
/etc/nginx/sites-enabled/90-suleyman.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/sites-enabled/90-suleyman.conf
    - makedirs: True
/etc/nginx/lua.init.d/init_solomon.lua:
  file.managed:
    - source: salt://{{ slspath }}/files/lua.init.d/init_solomon.lua
    - makedirs: True
  pkg.installed:
    - pkgs:
      - lua-cjson

/etc/nginx/lua/solomon-metrics.lua:
  file.managed:
    - source: salt://{{ slspath }}/files/lua/solomon-metrics.lua
    - makedirs: True

/etc/solomon/conf.d/weblog_lua.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/solomon.conf
    - makedirs: True
{% endif %}

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate-nginx
    - makedirs: True


### CLEANUP ###
cleanup old configs:
  file.absent:
    - names:
      - /etc/nginx/conf.d/lua.conf
      - /etc/nginx/conf.d/tskv.conf
