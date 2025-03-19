{% from "components/greenplum/map.jinja" import dep,sysvars with context %}
{% if salt.pillar.get('gpdb_master') %}
{% set role = 'master' %}
{% else %}
{% set role = 'segment' %}
{% endif %}

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
echo '#!/bin/bash' > {{ dep.rc_local }}:
  cmd.run:
    - unless: "head -1 {{ dep.rc_local }} | fgrep -q 'bash'"

Enabling-rc.local:
  service.running:
    - name: rc-local
    - enable: True
    - watch:
      - file: {{ dep.rc_local }}

Set_+x_rc.local:
  file.managed:
    - name: {{ dep.rc_local }}
    - mode: 0755

{%   for disk in salt.grains.get('disks') %}
{%     if not disk.startswith('md') %}
Set-io-scheduler-to-{{ disk }}-persistent:
  file.append:
    - name: {{ dep.rc_local }}
    - text:
      - /bin/echo {{ sysvars.ioscheduler }} > /sys/block/{{ disk }}/queue/scheduler

Set-io-scheduler-to-{{ disk }}:
  cmd.run:
    - name: /bin/echo {{ sysvars.ioscheduler }} > /sys/block/{{ disk }}/queue/scheduler
    - unless: "fgrep -q '[{{ sysvars.ioscheduler }}]' /sys/block/{{ disk }}/queue/scheduler"
{%     endif %}
{%   endfor %}

{%   for md in salt['grains.get']('mdadm', []) %}
disable_internal_bitmap_{{ md }}:
  cmd.run:
    - name: "mdadm --grow --bitmap=none /dev/{{ md }}"
    - onlyif:
      - "mdadm --detail /dev/{{ md }} | fgrep 'Intent Bitmap : Internal'"
      - "fgrep idle /sys/block/{{ md }}/md/sync_action"
{%   endfor %}
{% endif %}

# Fix bug in systemd-logind
/etc/systemd/logind.conf:
  file.replace:
    - pattern: '^(#RemoveIPC.*$|RemoveIPC.*$)'
    - repl: 'RemoveIPC=no'

systemd-logind:
  service.running:
    - watch:
      - file: /etc/systemd/logind.conf
