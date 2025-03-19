{% from "variables.sls" import masters with context %}

/etc/salt/minion:
  file.managed:
    - makedirs: True
    - mode: 644
    - template: jinja
    - source: salt://minion/minion.tpl
    - context:
      masters: {{ masters }}

/etc/cron.hourly/salt-copy-master-keys:
  file.managed:
    - mode: 755
    - source: salt://minion/salt-copy-master-keys

/usr/bin/salt-key-cleanup:
  file.managed:
    - mode: 755
    - source: salt://minion/salt-key-cleanup

salt-minion:
  pkg.installed:
  - pkgs:
    - salt-common
    - salt-minion
    - salt-yandex-components
  service.running:
    - enable: False
    - require:
      - pkg: salt-minion
      - file: /etc/salt/minion
    - watch:
      - file: /etc/salt/minion
      # this files listed in reomved below
      - file: /etc/salt/minion.d/auth_flood_fix.conf
      - file: /etc/salt/minion.d/minion.conf
      #- file: /etc/salt/pki/minion/master_sign.pub
      - file: /etc/salt/pki/minion/minion_master.pub

### cleanup
{% load_yaml as old_files %}
- /etc/salt/minion.d/auth_flood_fix.conf
- /etc/salt/minion.d/minion.conf
#- /etc/salt/pki/minion/master_sign.pub
- /etc/salt/pki/minion/minion_master.pub
- /etc/cron.hourly/yandex-media-common-salt-minion-meta
{% endload %}
{% for of in old_files %}
{{of}}:
  file.absent: []
{% endfor %}
