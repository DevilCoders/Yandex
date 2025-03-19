yc-snapshot:
  yc_pkg.installed:
    - pkgs:
      - yc-snapshot
      - qemu-utils
      - qemu-block-extra
  service.running:
    - enable: True
    - require:
      - cmd: yc-snapshot-populate-database
      - yc_pkg: yc-snapshot
      - file: /etc/yc/snapshot/config.toml
    - watch:
      - file: /etc/yc/snapshot/config.toml
      - yc_pkg: yc-snapshot

yc-snapshot-gc:
  yc_pkg.installed:
    - pkgs:
      - yc-snapshot
      - qemu-utils
      - qemu-block-extra
  service.running:
    - enable: True
    - name: yc-snapshot-gc.timer
    - require:
      - cmd: yc-snapshot-populate-database
      - yc_pkg: yc-snapshot
      - file: /etc/yc/snapshot/config.toml
    - watch:
      - file: /etc/yc/snapshot/config.toml
      - yc_pkg: yc-snapshot

/etc/yc/snapshot/config.toml:
  file.managed:
    - source: salt://{{ slspath }}/config.toml
    - template: jinja
    - user: root
    - group: root
    - makedirs: True

yc-snapshot-populate-database:
  cmd.run:
    - name: /usr/bin/yc-snapshot-populate-database --config=/etc/yc/snapshot/config.toml
    - require:
      - file: /etc/yc/snapshot/config.toml
      - yc_pkg: yc-snapshot
    - onchanges:
      - yc_pkg: yc-snapshot

{% if salt['grains.get']('overrides:snapshot_mode') == 'test' %}
{# Run Snapshot in local mode in which all data is stored locally #}
{# instead of KiKiMR, which significantly speeds up tests execution. #}

local_snapshot_data_dir:
  file.directory:
    - name: /var/lib/yc/snapshot/data
    - user: yc-snapshot
    - group: yc-snapshot
    - makedirs: True
    - require_in: yc-snapshot

{% endif %}

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
