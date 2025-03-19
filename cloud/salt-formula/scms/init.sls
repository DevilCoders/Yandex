yc-scms:
  yc_pkg.installed:
    - pkgs:
      - yc-scms
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-scms
      - file: /etc/yc/scms/config.toml
      - file: /var/lib/yc/scms/system_account.json
    - watch:
      - file: /etc/yc/scms/config.toml
      - yc_pkg: yc-scms

/etc/yc/scms/config.toml:
  file.managed:
    - source: salt://{{ slspath }}/config.toml
    - template: jinja
    - require:
      - yc_pkg: yc-scms
      - file: /var/lib/yc/scms/master.key

/var/lib/yc/scms/master.key:
  file.managed:
    - source: salt://{{ slspath }}/master.key
    - user: yc-scms
    - mode: '0400'
    - makedirs: True
    - replace: False
    - require:
      - yc_pkg: yc-scms

/var/lib/yc/scms/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/system_account.json
    - user: root
    - group: yc-scms
    - mode: '0040'
    - makedirs: True
    - replace: False
    - require:
      - yc_pkg: yc-scms

scms-populate-db:
  cmd.run:
    - name: '/usr/bin/yc-scms-populate-db'
    - require:
      - file: /etc/yc/scms/config.toml
    - onchanges:
      - yc_pkg: yc-scms

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
