{% import "common/apparmor/init.jinja" as yc_aa %}
{% set aa_qemu_mode = pillar["apparmor"]["qemu"] %}

include:
  - common.apparmor

apparmor_qemu_profile:
  file.managed:
    - name: {{ yc_aa.policies_dir }}/qemu
    - source: salt://{{ slspath }}/files/apparmor/qemu
    - user: root
    - group: root
    - mode: '0640'
    - template: jinja
    - replace: True
    - defaults:
      profile_flags: {{ aa_qemu_mode }}
    - require:
      - file: yc_apparmor_policies_files_dir

apparmor_qemu_profile_files:
  file.recurse:
    - name: {{ yc_aa.policies_files_dir }}
    - source: salt://{{ slspath }}/files/apparmor/yc/
    - user: root
    - group: root
    - dir_mode: 1750
    - file_mode: '0640'
    - replace: True
    - require:
      - file: yc_apparmor_policies_files_dir
      - file: apparmor_qemu_profile

apparmor_qemu_profile_applied:
  cmd.run:
    - name: |-
        'aa-{{ aa_qemu_mode }}' '{{ yc_aa.policies_dir }}/qemu'
    - require:
      - file: apparmor_qemu_profile
      - service: apparmor_service

# It's the security. We must ensure profile is loaded as we wanted
apparmor_qemu_profile_ensured:
  cmd.run:
    - name: grep -m1 -x '/usr/bin/qemu-system-x86_64 ({{ aa_qemu_mode }})' /sys/kernel/security/apparmor/profiles
    - require:
      - service: apparmor_service

apparmor_qemu_profile_active:
  test.nop:
    - require:
      - cmd: apparmor_qemu_profile_applied
      - cmd: apparmor_qemu_profile_ensured
    - require_in:
      - service: yc-compute-node
