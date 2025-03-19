{% set default_zone_id = pillar['placement']['dc'] %}
{% set zones = salt['grains.get']('cluster_map:availability_zones', [default_zone_id]) %}

{% for zone_id in zones %}

yc_compute_worker_{{ zone_id }}_stop:
  service.dead:
    - name: yc-compute-worker@{{ zone_id }}
    - enable: False

{% endfor %}

yc-compute-api:
  service.dead:
    - enable: False
