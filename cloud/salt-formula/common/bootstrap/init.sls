{%- set stand_type = grains['cluster_map']['stand_type'] %}

{# NOTE(CLOUD-17174): Only add cron job on hardware, because in CI, it is useless and might break
   pull-cluster-map test #}
{%- if stand_type == 'hardware' %}
cron-cluster-map-update:
  cron.present:
    - identifier: cron-cluster-map-update
    - name: "/usr/bin/yc-ci-remote pull-cluster-map"
    - user: root
{% endif %}

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
