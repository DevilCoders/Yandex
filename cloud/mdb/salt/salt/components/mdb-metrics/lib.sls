{% from "components/mdb-metrics/map.jinja" import mdb_metrics with context %}

{% set mdb_metrics_path = 'components/mdb-metrics' %}
{% macro deploy_configs(conf_type, conf_dir=mdb_metrics_path) %}

{% set configs = mdb_metrics['configs'][conf_type] %}
{% for conf in configs %}
mdb-metrics-{{ conf }}-config:
    file.managed:
        - name: /etc/mdb-metrics/conf.d/available/{{ conf }}.conf
        - template: jinja
        - source: salt://{{ conf_dir }}/conf/{{ conf_type }}/{{ conf }}.conf
        - watch_in:
            - service: mdb-metrics-service
        - defaults:
            is_burst: {{ mdb_metrics['is_burst'] }}
            min_burst_interval: {{ mdb_metrics['min_burst_interval'] }}
{% if conf_type in ('pg_unmanaged',) %}
            databases: {{ mdb_metrics['unmanaged_databases'] }}
{% endif %}
mdb-metrics-{{ conf }}-link:
    file.symlink:
        - name: /etc/mdb-metrics/conf.d/enabled/{{ conf }}.conf
        - target: /etc/mdb-metrics/conf.d/available/{{ conf }}.conf
        - watch_in:
            - service: mdb-metrics-service
{% endfor %}
{% endmacro %}

{% macro yasmgetter_config(rule_name, getter_file='/usr/local/yasmagent/default_getter.py') %}
{%     set accumulator_filename = '/lib/systemd/system/yasmagent.service' %}
{% if salt['pillar.get']('data:use_yasmagent', True) %}
{{ rule_name }}:
    file.accumulated:
        - name: yasmagent-instance-getter
        - filename: {{ accumulator_filename }}
        - text: {{ getter_file }}
        - require_in:
              - file: {{ accumulator_filename }}
        - watch_in:
              - service: yasmagent
{% endif %}
{% endmacro %}
