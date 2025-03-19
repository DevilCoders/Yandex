{% if "stress" in  grains["yandex-environment"] %}
  {% set masters_group = "ape-test-salt" %}
{% else %}
  {% set masters_group = "ape-salt" %}
{% endif %}

{% set masters = salt.conductor.groups2hosts(masters_group) %}
{% set robot = "robot-media-salt" %}
