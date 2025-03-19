{%- from slspath + "/map.jinja" import mm_vars with context -%}
{%- set env =  grains['yandex-environment'] %}
{%- if env == 'testing' %}

/etc/nscfg/resource-provider-develop.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/resource-provider-develop.conf.yaml
    - template: jinja
    - context:
      vars: {{ mm_vars }}

/etc/nscfg/resource-provider-tvm-secret-develop:
  file.managed:
    - owner: resource-provider
    - mode: 600
    - contents: {{ pillar.get("resource-provider-tvm-secret-develop") }}

/etc/ubic/service/resource-provider-develop.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/resource-provider-develop.json

/etc/logrotate.d/resource-provider-develop:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/resource-provider-develop
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/resource-provider-develop:
  file.directory:
    - mode: 755
    - user: resource-provider
    - group: resource-provider
    - makedirs: True

resource-provider-develop-status:
  monrun.present:
    - command: "curl localhost:8667/ping > /dev/null 2>&1 && echo '0;OK' || echo '2;Failed'"
    - execution_interval: 60
    - execution_timeout: 10
    - type: mastermind

{%- endif %}

/etc/nscfg/resource-provider.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/resource-provider.conf.yaml
    - template: jinja
    - context:
      vars: {{ mm_vars }}

/etc/nscfg/resource-provider-tvm-secret:
  file.managed:
    - owner: resource-provider
    - mode: 600
    - contents: {{ pillar.get("resource-provider-tvm-secret") }}

/etc/ubic/service/resource-provider.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/resource-provider.json

/etc/logrotate.d/resource-provider:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/resource-provider
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/resource-provider:
  file.directory:
    - mode: 755
    - user: resource-provider
    - group: resource-provider
    - makedirs: True

resource-provider-status:
  monrun.present:
    - command: "curl localhost:9778/ping > /dev/null 2>&1 && echo '0;OK' || echo '2;Failed'"
    - execution_interval: 60
    - execution_timeout: 10
    - type: mastermind
