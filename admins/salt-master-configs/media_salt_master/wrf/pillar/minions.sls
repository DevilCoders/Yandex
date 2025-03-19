{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' or yaenv == 'prestable' or yaenv == 'qa' %}
  {% set group = 'wrf-stable-salt' %}
{% else %}
  {% set group = 'wrf-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
