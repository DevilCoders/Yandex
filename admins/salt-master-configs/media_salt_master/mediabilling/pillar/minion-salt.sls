{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' or yaenv == 'prestable' or yaenv == 'qa' %}
    {% set group = 'mediabilling-stable-salt' %}
{% else %}
    {% set group = 'mediabilling-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
