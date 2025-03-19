{%- from slspath + "/map.jinja" import mm_vars with context -%}

{% if pillar.get('is_collector', False) %}
/etc/nginx/sites-available/20-drooz.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-available/20-drooz.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - template: jinja
    - context:
      vars: {{ mm_vars }}
    - watch_in:
      - service: nginx_drooz

/etc/nginx/sites-enabled/20-drooz.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/20-drooz.conf
    - makedirs: true
    - require:
      - file: /etc/nginx/sites-available/20-drooz.conf

/etc/elliptics/mastermind-drooz-v2.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind-drooz-v2.conf
    - template: jinja
    - context:
      vars: {{ mm_vars }}

mastermind-drooz_v2:
  monrun.present:
    - command: "/usr/bin/http_check.sh ping {{ mm_vars.drooz_v2.http_port }}"
    - execution_interval: 60
    - execution_timeout: 30
    - type: mastermind
    - makedirs: True

nginx_drooz:
  service:
    - name: nginx
    - running
    - reload: True
{% endif %}
