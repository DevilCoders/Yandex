{% set yaenv = grains['yandex-environment'] %}
{% set c_group = salt['grains.get']('conductor:group') %}
{% if yaenv in ['production', 'prestable', 'qa'] or c_group in ['weather_dom0-testing', 'weather_dom0-load', 'weather_dom0-dev'] %}
  {% set group = 'weather-stable-salt' %}
{% else %}
  {% set group = 'weather-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
