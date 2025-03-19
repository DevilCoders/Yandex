{% from "components/mdb-metrics/map.jinja" import mdb_metrics with context %}
{% set porto_configs = mdb_metrics['configs']['porto'] %}
{% for conf in porto_configs %}
mdb-metrics-{{ conf }}-config:
    file.managed:
        - name: /etc/mdb-metrics/conf.d/available/{{ conf }}.conf
        - template: jinja
        - source: salt://{{ slspath }}/conf/porto/{{ conf }}.conf
        - watch_in:
            - service: mdb-metrics-service
        - require:
            - file: /etc/mdb-metrics/conf.d/available
        - defaults:
            is_burst: {{ mdb_metrics['is_burst'] }}
            min_burst_interval: {{ mdb_metrics['min_burst_interval'] }}

mdb-metrics-{{ conf }}-link:
    file.symlink:
        - name: /etc/mdb-metrics/conf.d/enabled/{{ conf }}.conf
        - target: /etc/mdb-metrics/conf.d/available/{{ conf }}.conf
        - watch_in:
            - service: mdb-metrics-service
        - require:
            - file: /etc/mdb-metrics/conf.d/enabled
{% endfor %}
