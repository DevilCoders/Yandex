{% set env = grains['yandex-environment'] %}
include:
  - .{{env}}
