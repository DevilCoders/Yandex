{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' %}
    {% set group = 'media-stable-salt' %}
{% else %}
    {% set group = 'media-test-salt' %}
{% endif %}
salt_minion:
  lookup:
    masters_group: {{ group }}
