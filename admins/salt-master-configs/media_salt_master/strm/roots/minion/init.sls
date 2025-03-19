{% from "variables.sls" import m with context %}
# dirty hack
{% set ctype = grains['yandex-environment'] %}
{% set default_env = 'stable' if ctype == 'production' else ctype %}
{% if 'strm-prestable' in grains['c'] %}
'/etc/yandex-pkgver-ignore.d/yandex-environment-production':
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: 'yandex-environment-production'
yandex-environment-prestable:
  pkg:
    - installed
{% set ctype = 'prestable' %}
{% set default_env = 'prestable' %}
{% endif %}

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

### cleanup
{% load_yaml as old_files %}
- /etc/salt/minion.d/auth_flood_fix.conf
- /etc/salt/minion.d/minion.conf
- /etc/salt/pki/minion/master_sign.pub
- /etc/salt/pki/minion/minion_master.pub
- /etc/cron.hourly/yandex-media-common-salt-minion-meta
- /usr/bin/salt-key-cleanup
{% endload %}
{% for of in old_files %}
{{of}}:
  file.absent: []
{% endfor %}
