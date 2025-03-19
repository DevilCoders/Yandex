{% set yaenv = grains['yandex-environment'] %}

{% for upstream_name, args in salt['pillar.get']('nginx_configs:upstreams').items() %}
{% set _default_upstream_ = args.get('default') %}
{% if yaenv == 'testing' %}
{% set upstream = args.get('testing', {}) %}
{% elif yaenv == 'prestable' %}
{% set upstream = args.get('prestable') %}
{% elif yaenv == 'development' %}
{% set upstream = args.get('development') %}
{% elif yaenv == 'production' %}
{% set upstream = args.get('production') %}
{% else %}
{% set upstream = args.get('stress') %}
{% endif %}

{% if upstream is none %}
{% set upstream = _default_upstream_ %}
{% endif %}

{% set common = _default_upstream_ %}
{% do common.update(upstream) %}

{{ upstream_name }}:
    file.managed:
      - name: /etc/nginx/sites-available/20-{{ upstream_name }}.conf
      - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/local_configs.conf.tpl
      - template: jinja
      - context:
        upstream_name: {{ upstream_name }}

        logtype: {{ common.get('logtype') }}
        server: {{ common.get('server') }}
        logname: {{ common.get('logname')  }}-access.log
        proxy_read_timeout: {{ common.get('proxy_read_timeout') }}
        proxy_connect_timeout: {{ common.get('proxy_connect_timeout') }}
        port: {{ common.get('port') }}
        schema: {{ common.get('schema') }}
        server_port: {{ common.get('server_port') }}

symlink_{{ upstream_name }}:
    file.symlink:
      - target: /etc/nginx/sites-available/20-{{ upstream_name }}.conf
      - name: /etc/nginx/sites-enabled/20-{{ upstream_name }}.conf
      - force: True

{% endfor %}
