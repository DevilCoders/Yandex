{%- import "common/kikimr/init.sls" as kikimr_vars with context %}

{% set hostname = grains['nodename'] %}

# In cloudvm we have only one AZ, so no special workers neeeded
{% if grains['cluster_map']['hosts'][hostname]['base_role'] != "cloudvm" %}

{% set default_zone_id = pillar['placement']['dc'] %}
{% set kikimr_clusters = salt['grains.get']('cluster_map:kikimr:clusters', {}) %}

include:
    - compute

  {% for cluster in kikimr_clusters %}
    {% if kikimr_vars.nbs_database in kikimr_clusters[cluster]['dynamic_nodes'] %}
      {% set nbs_host = kikimr_clusters[cluster]['dynamic_nodes'][kikimr_vars.nbs_database]|sort|first %}
      {% set zone_id = salt['grains.get']('cluster_map:hosts:%s:location:zone_id' % nbs_host, default_zone_id) %}

/etc/yc/compute/config-{{ zone_id }}.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/config.yaml
    - require:
      - yc_pkg: yc-compute
    - context:
      nbs_host: {{ nbs_host }}
      zone_id: {{ zone_id }}

/etc/yc/compute/{{ zone_id }}.env:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/zone.env
    - require:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config-{{ zone_id }}.yaml
    - context:
      config: /etc/yc/compute/config-{{ zone_id }}.yaml
      port: {{ 9100 + loop.index0 }}

yc-compute-worker@{{ zone_id }}:
  service.running:
    - enable: True
    - require:
      - cmd: populate-devel-database
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/{{ zone_id }}.env
    - watch:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config-{{ zone_id }}.yaml

    {% endif %}
  {% endfor %}
{% endif %}
