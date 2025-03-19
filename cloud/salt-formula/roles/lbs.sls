{%- set stand_type = grains['cluster_map']['stand_type'] %}
include:
  - lbs
{% if stand_type != 'virtual' %}
  - tests.lbs
{%- endif %}
