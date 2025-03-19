{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' or yaenv == 'prestable' or yaenv == 'qa' %}
    {% set group = 'music-stable-salt' %}
{% else %}
    {% set group = 'music-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
