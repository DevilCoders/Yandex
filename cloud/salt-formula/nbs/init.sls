include:
  - z2
  - .client
  - .server

{% from slspath+"/monitoring.yaml" import monitoring -%}
{% include "common/deploy_mon_scripts.sls" %}
