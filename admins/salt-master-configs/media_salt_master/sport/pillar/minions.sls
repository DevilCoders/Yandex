{% set yaenv = grains['yandex-environment'] %}
{% set c_group = salt['grains.get']('conductor:group') %}
{% if yaenv == 'production' or yaenv == 'prestable' or yaenv == 'qa' or c_group in ['sport-load-dom0'] %}
  {% set group = 'sport-stable-salt' %}
{% else %}
  {% set group = 'sport-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
