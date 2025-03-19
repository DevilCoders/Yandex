kvm:
  group.present:
    - system: True

{%- set stand_type = grains['cluster_map']['stand_type'] %}
{%- if stand_type != 'virtual' %}
{# ensure we can use vfio-pci #}
/etc/udev/rules.d/vfio-pci.rules:
  file.managed:
    - contents: 'SUBSYSTEM=="vfio", OWNER="root", GROUP="kvm"'

trigger-udev:
  cmd.run:
    - names:
      - /sbin/udevadm control --reload-rules
      - /sbin/udevadm trigger --subsystem-match=vfio
    - onchanges:
      - file: /etc/udev/rules.d/vfio-pci.rules
    - require:
      - group: kvm

{# ensure memlock limits for qemu #}
/etc/security/limits.d/yc_memlock.conf:
  file.managed:
    - contents:
      - yc-compute-node soft memlock unlimited
      - yc-compute-node hard memlock unlimited
{%- endif %}
