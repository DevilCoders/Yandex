include:
  - .sriov
  - .yc-compute-node

{%- import_yaml slspath + "/monitoring.yaml" as monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
