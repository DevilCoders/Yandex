{% set unit = 'yandex-hbf-agent' %}

yandex-hbf-agent-static:
  pkg.installed:
    - pkgs: {{ salt['conductor.package']('yandex-hbf-agent-static') }}

/etc/yandex-hbf-agent/rules.d/48-localoutblock.v4:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex-hbf-agent/rules.d/48-localoutblock.any
    - mode: 644
    - user: root
    - group: root
    - makedirs: False

/etc/yandex-hbf-agent/rules.d/48-localoutblock.v6:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex-hbf-agent/rules.d/48-localoutblock.any
    - mode: 644
    - user: root
    - group: root
    - makedirs: False
