{% from slspath + "/map.jinja" import push_client with context %}
{% set is_prod = salt['grains.get']('yandex-environment') in ['production'] %}
include:
  - .services

/etc/logrotate.d/push-client:
{% if push_client.stats %}
  file.managed:
    - source: {{ push_client.logrotate }}
    - template: jinja
    - require:
      - pkg: push_client_packages
    - context:
        stats: {{ push_client.stats }}
        init: {{ salt["grains.get"]("init") }}
{% else %}
  file.absent
{% endif %}

{%- if push_client.clean_push_client_configs %}
{%- for cfg in push_client.not_managed_cfgs %}
{{ cfg }}:
  file.absent:
    - require_in:
      - file: /etc/default/push-client
{%- endfor %}
{%- endif %}

{%- if salt["pillar.get"]('push_client:ignore_pkgver', false) %}
/etc/yandex-pkgver-ignore.d/yandex-push-client:
  file.managed:
    - makedirs: True
    - mode: 644
    - replace: False
    - contents: yandex-push-client
{%- endif %}

/etc/default/push-client:
  file.managed:
    - source: salt://templates/push-client/files/etc/default/push-client
    - follow_symlinks: False
    - template: jinja
    - require:
      - pkg: push_client_packages
    - watch_in:
      - service: {{ push_client.service }}

{%- for st in push_client.stats %}
{{ st.conf }}:
  file.managed:
    - source: salt://templates/push-client/files/etc/yandex/statbox-push-client/config.tmpl
    - template: jinja
    - makedirs: True
    - context:
        stat:    {{ st }}
        host:    {{ salt["grains.get"]("conductor:fqdn")    }}
        group:   {{ salt["grains.get"]("conductor:group")   }}
        logs:    {{ st.get("logs", push_client.get("logs", [])) }}
    - require_in:
      - file: /etc/default/push-client
    - watch_in:
      - service: {{ push_client.service }}

/var/lib/{{ st.name }}:
  file.directory:
    - user: statbox
    - group: statbox
    - dir_mode: "0755"
    - require:
      - pkg: push_client_packages
{%- endfor %}

{% if salt['pillar.get']('push_client:stats:tvm_secret') %}
/etc/yandex/statbox-push-client/{{ salt['pillar.get']('push_client:stats:tvm_secret_file', ".tvm_secret") }}:
  file.managed:
    - source: salt://templates/push-client/files/etc/yandex/statbox-push-client/tvm_secret
    - template: jinja
    - user: statbox
    - mode: 640
    - context:
        secret: {{ salt['pillar.get']('push_client:stats:tvm_secret') }}
    - watch_in:
      - service: {{ push_client.service }}
    - require:
      - pkg: push_client_packages
{% endif %}

