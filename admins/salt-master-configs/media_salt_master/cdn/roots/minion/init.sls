{% from "variables.sls" import m with context %}
# dirty hack
{% set ctype = grains['yandex-environment'] %}
{% set default_env = 'stable' if grains['yandex-environment'] == 'production' else 'prestable' %}

/etc/salt/minion:
  file.managed:
    - makedirs: True
    - mode: 644
    - template: jinja
    - source: salt://minion/minion.tpl
    - context:
        masters: {{ salt.conductor.groups2hosts(m.masters_group) }}
        ctype: {{ ctype }}
        geo: {{ grains['conductor']['root_datacenter'] }}
        default_env: {{ default_env }}

salt-minion:
  service.dead:
    - enable: False
