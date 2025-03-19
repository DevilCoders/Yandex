{% set unit = 'yandex-hbf-agent' %}

yandex-hbf-agent-static:
  pkg.installed:
    - pkgs: {{ salt['conductor.package']('yandex-hbf-agent-static') }}