{%- set yaenv = grains['yandex-environment'] %}
include:
    - mongodb.{{ yaenv }}
