{%- set environment = grains['cluster_map']['environment'] %}
{%- set hostname = grains['nodename'] %}
{%- set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}

{# FIXME(CLOUD-21260): hw-labs do not have api adapter, can't install iam-agent #}
{% if environment != 'dev' or base_role == 'cloudvm' %}

iam-agent:
  yc_pkg.installed:
    - pkgs:
      - yc-token-agent
  service.running:
    - name: yc-token-agent
    - enable: True
    - require:
      - yc_pkg: iam-agent
    - watch:
      - file: /etc/yc/token-agent/config.yaml

/etc/yc/token-agent/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/config.yaml
    - require:
      - yc_pkg: iam-agent

{% endif %}
