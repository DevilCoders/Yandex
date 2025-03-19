{%- set resources_set_name = grains['cluster_map']['resources'] -%}
{%- set all_resources = pillar['list_resources'] -%}

{%- set additional_resources_set = {
    'subnets' : all_resources.get('subnets', {}).keys(),
    'fip_buckets': all_resources.get('fip_buckets', {}).keys(),
} -%}

{%- include 'common/get_resources.sls' %}
