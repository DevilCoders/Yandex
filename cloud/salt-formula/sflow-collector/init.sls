#jinja2: trim_blocks:True, lstrip_blocks:True

include:
    - common.push-client
    - .network
    - .gobgp
    - .tflow

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}