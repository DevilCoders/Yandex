{%- set roles = grains['cluster_map']['hosts'][grains['nodename']]['roles'] %}
{%- set hostname = grains['nodename'] -%}
{%- set host_tags = grains['cluster_map']['hosts'][hostname].get('tags', []) -%}

solomon-packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-solomon-agent-bin
      - yc-solomon-agent-systemd
      - yc-solomon-agent-plugins

solomon-agent-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/agent.conf
    - template: jinja
    - source: salt://{{ slspath }}/agent.conf
    - require:
      - yc_pkg: solomon-packages

solomon-agent-systemd-memory-limit:
  file.managed:
    - name: /etc/systemd/system/solomon-agent.service.d/10-memory-limit.conf
    - source: salt://{{ slspath }}/files/10-memory-limit.conf
    - makedirs: True
    - require:
      - yc_pkg: solomon-packages

solomon-agent-systemd-secrets-conf:
  file.managed:
    - name: /lib/systemd/system/solomon-agent.service.d/20-secrets.conf
    - source: salt://{{ slspath }}/files/20-secrets.conf
    - makedirs: True
    - require:
      - yc_pkg: solomon-packages

solomon-agent-reload-unit:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: solomon-agent-systemd-secrets-conf

solomon-agent-system-plugin-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/system-plugin.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/system-plugin.conf
    - require:
      - yc_pkg: solomon-packages

solomon-agent-nbs-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/nbs-metrics.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/nbs-metrics.conf
    - require:
      - yc_pkg: solomon-packages

{%- if 'compute' in roles %}
solomon-agent-serverless-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/serverless-engine.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/serverless-engine.conf
    - require:
      - yc_pkg: solomon-packages
{%- endif %}

{%- if grains["cluster_map"]["environment"] == "prod" %}
solomon-agent-log-reader-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/log-reader.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/log-reader.conf
    - require:
      - yc_pkg: solomon-packages
{%- endif %}

{%- if 'loadbalancer-node' in roles %}
solomon-agent-ylb-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/ylb-metrics.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/ylb-metrics.conf
    - require:
      - yc_pkg: solomon-packages

solomon-agent-ylb-billbro-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/ylb-billbro-metrics.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/ylb-billbro-metrics.conf
    - require:
      - yc_pkg: solomon-packages

solomon-agent-ylb-vsop-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/ylb-vsop.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/ylb-vsop.conf
    - require:
      - yc_pkg: solomon-packages
{%- endif %}

{%- if 'bgp2vpp' in host_tags %}
solomon-agent-bgp2vpp-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/yc-bgp2vpp.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/yc-bgp2vpp.conf
    - require:
      - yc_pkg: solomon-packages
{%- endif %}

{% set prometheus_configs = pillar.get('solomon-agent', {}).get('prometheus-plugin', {}) %}
{% for role in roles %}
  {% if role in prometheus_configs %}
    {% for service_name, service_config in prometheus_configs[role].iteritems() %}

solomon-{{ role }}-{{ service_name }}-prometheus-plugin-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/{{ role }}-{{ service_name }}-prometheus.conf
    - source: salt://{{ slspath }}/files/prometheus-plugin.conf
    - template: jinja
    - context:
        service_name: {{ service_name }}
        pull_interval: {{ service_config['pull-interval'] }}
        url: {{ service_config['url'] }}

    {% endfor %}
  {% endif %}
{% endfor %}

#FIXME: Remove this SLS when SOLOMON-2842 will be closed. CLOUD-12012
solomon-agent-plugin-cleanup:
  cmd.run:
    - name: 'find /usr/lib/python2.7/dist-packages/yc_solomon_plugins -name "*.pyc" -delete'
    - require:
      - yc_pkg: solomon-packages
    - onchanges:
      - yc_pkg: solomon-packages

solomon-agent:
  service.running:
    - enable: true
    - require:
      - file: solomon-agent-config
      - yc_pkg: solomon-packages
    - watch:
      - file: solomon-agent-config
      - file: solomon-agent-system-plugin-config
      - file: solomon-agent-nbs-config
      - file: solomon-agent-systemd-memory-limit
      - file: solomon-agent-systemd-secrets-conf
      - yc_pkg: solomon-packages

{%- if 'compute' in roles or 'oct_head' in roles %}
repost-juggler-solomon-files:
  file.managed:
    - names:
      - /etc/systemd/system/repost-juggler-solomon.service:
        - source: salt://{{ slspath }}/files/repost-juggler-solomon.service
      - /usr/local/bin/repost-juggler-solomon.py:
        - source: salt://{{ slspath }}/bin/repost-juggler-solomon.py
        - mode: 0755

repost-juggler-solomon:
  service.running:
    - enable: true
    - watch:
      - file: repost-juggler-solomon-files
    - require:
      - service: solomon-agent
{%- endif %}

solomon-sysmond-pkgs:
  yc_pkg.installed:
    - pkgs:
      - yandex-solomon-sysmond

solomon-sysmond-service:
  service.running:
    - name: yandex-solomon-sysmond
    - enable: true
    - require:
      - yc_pkg: solomon-sysmond-pkgs
    - watch:
      - yc_pkg: solomon-sysmond-pkgs

{# NOTE(k-zaitsev): CLOUD-14198, actual check is called solomon-agent.
   This state is to cleanup leftover monrun state
#}
/etc/monrun/conf.d/solomon_check.conf:
  file.absent

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
