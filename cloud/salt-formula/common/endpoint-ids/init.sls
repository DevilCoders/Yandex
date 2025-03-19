{% set environment = grains['cluster_map']['environment'] %}

{% set cluster_map_zones = grains["cluster_map"]["availability_zones"] %}
{% set filename = "{}/files/ids-{}.yaml".format(slspath, environment) %}
{% import_yaml filename as ids_yaml %}

endpoint_ids_check:
  module.run:
    - name: endpoint_ids.check
    - cluster_map_zones: {{ cluster_map_zones }}
    - ids_yaml: {{ ids_yaml }}
    - filename: {{ filename }}

/etc/yc/ids.yaml:
  file.managed:
    - source: salt://{{ filename }}
    - makedirs: True
    - mode: 0644
    - require:
        - module: endpoint_ids_check
