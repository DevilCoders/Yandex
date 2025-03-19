{% set yaenv = grains['yandex-environment'] -%}

include:
  - .mysql-sessions
  - templates.pt-stalk
  - templates.pt-deadlock-logger
  - templates.pt-fk-error-logger
  - .monrun
  - .mysql-configurator-4
{% if yaenv in ['testing', 'production'] %}
  - .pt-scripts
  - templates.sudoers
  - .mysql-wrapper
{% endif %}

lzop_package:
  pkg.installed:
    - name: lzop
