lua-cjson:
  pkg.installed:
    - pkgs:
      - lua-cjson

/etc/nginx/conf.d/01-lua-packages.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/conf.d/01-lua-packages.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - watch_in:
        - service: nginx

/etc/nginx/conf.d/10-unistat.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/conf.d/10-unistat.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - watch_in:
        - service: nginx

{% for filename in ['base-metrics', 'metric-funcs', 'worker'] %}
/etc/nginx/include/{{filename}}.lua:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/{{filename}}.lua
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - watch_in:
        - service: nginx
{% endfor %}
