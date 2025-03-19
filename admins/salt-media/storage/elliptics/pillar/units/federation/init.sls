{%- set conductor_group = grains['conductor']['group'] -%}
{% if '-federation-' in conductor_group -%}
mds_federation: {{ conductor_group.split('-')[-1] }} 
{%- else -%}
mds_federation: 1
{%- endif %}
{%- if grains['yandex-environment'] in ['testing'] %}
include:
  - units.federation.federations-testing
{%- else %}
include:
  - units.federation.federations
{%- endif %}
