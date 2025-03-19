{%- from "common/push-client/map.jinja" import push_client with context %}
{%- import_yaml slspath+"/mon/push-client.yaml" as monitoring %}

{% if push_client.get('enabled', False) and push_client.get('instances', {}).values() | selectattr("enabled", "defined") | selectattr("enabled") | selectattr("files", "defined") | selectattr("files") | list %}

push-client:
  yc_pkg.installed:
    - name: yandex-push-client
  service.running:
    - name: statbox-push-client
    - enable: True
    - require:
      - yc_pkg: push-client

/etc/default/push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/init.d
    - template: jinja
    - require:
      - yc_pkg: push-client
    - watch_in:
      - service: statbox-push-client

/etc/logrotate.d/push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/push-client.logrotate
    - require:
      - yc_pkg: push-client

{% for name, instance in push_client.instances.items() %}
{% if instance.get('enabled', False) and instance.get('files', []) %}
{# create config file of the push-client instance #}
/etc/yandex/statbox-push-client/conf.d/push-client-{{ name }}.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/push-client.yaml
    - template: jinja
    - makedirs: True
    - context:
      instance_name: {{ name }}
    - user: statbox
    - group: statbox
    - watch_in:
      - service: statbox-push-client
    - require:
      - yc_pkg: push-client

{# create a state directory #}
/var/lib/push-client/{{ name }}/:
  file.directory:
    - makedirs: True
    - user: statbox
    - group: statbox
    - require_in:
      - file: /etc/yandex/statbox-push-client/conf.d/push-client-{{ name }}.yaml
    - require:
      - yc_pkg: push-client

{# ensure that all log dirs exists #}
{% for file in instance.files %}
{{ name }}_{{ file.name }}:
  file.directory:
    - name: {{ salt.file.dirname(file.name) }}
    - makedirs: True
    - require_in:
      - service: statbox-push-client
      - file: /etc/yandex/statbox-push-client/conf.d/push-client-{{ name }}.yaml

{% endfor %}

{% set tvm = salt['pillar.get']('push_client:instances:%s:tvm' % name, default=push_client.get('defaults', {}).get('tvm', {}), merge=True) %}
{# tvm secret delivery #}
{% if tvm.get('enabled', False) %}
push_client_tvm_secret_{{ name }}:
  file.managed:
    - name: {{ tvm.get('secret_file', '/var/lib/yc/push-client/tvm.secret') }}
    - source: salt://{{ slspath }}/files/tvm.secret
    - user: statbox
    - mode: '0400'
    - makedirs: True
    - replace: False
    - require_in:
      - service: statbox-push-client
{% endif %}

{% endif %}
{% endfor %}

{%- include "common/deploy_mon_scripts.sls" -%}
{% endif %}
