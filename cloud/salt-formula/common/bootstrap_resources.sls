{%- set resources_set_name = grains['cluster_map']['bootstrap_resources'] -%}
{%- include 'common/get_resources.sls' %}
