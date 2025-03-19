include:
  - common.autorecovery
  - compute-node-vpc.common
  - .yc-network-billing-collector

{%- import_yaml slspath + "/monitoring.yaml" as monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
