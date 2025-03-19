{%- from slspath + "/map.jinja" import yarl_vars with context -%}

include:
  - .common

/etc/yarl/yarl.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yarl/yarl.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - context:
      vars: {{ yarl_vars }}

/etc/yarl/nginx.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yarl/nginx.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - context:
      vars: {{ yarl_vars }}
