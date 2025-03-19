yc-compute-node:
  yc_pkg.installed:
    - pkgs:
      - apparmor-ycloud-dhcp-prof
      - yc-compute-node
      - qemu
      - qemu-kvm
      - qemu-block-extra
      - qemu-system-common
      - qemu-system-x86
      - qemu-utils
      - seabios
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/compute-node/config.yaml
      - file: /var/lib/yc/compute-node/system_account.json
      - yc_pkg: yc-compute-node
      - mount: vm_memory

vm_memory:
  mount.mounted:
    - name: /dev/shm
    - device: tmpfs
    - fstype: tmpfs
    - mkmnt: True
    - opts:
      - rw,nosuid,nodev,size=100%,huge=advise
    - require:
      - yc_pkg: yc-compute-node

{% if grains['virtual'] == "physical" %}
  {% set vsock_map = {"present":["vhost_vsock"], "absent": ["vmw_vsock_vmci_transport"]} %}
vsock_udev_rules:
  file.managed:
    - name: /lib/udev/rules.d/60-open-vm-tools.rules
    - contents: 'KERNEL=="vhost-vsock", MODE="0600", GROUP="yc-compute-node", OWNER="yc-compute-node"'
    - require:
      - yc_pkg: yc-compute-node
  {% for action, modules in vsock_map.items() %}
physical_{{ action }}_modules:
  kmod.{{ action }}:
    - mods: {{ modules }}
    - persist: True
    - require:
      - file: vsock_udev_rules
    {% if action == "absent" %}
      {% for module in modules %}
blacklist-{{ module }}.conf:
  file.managed:
    - name: /etc/modprobe.d/blacklist-{{ module }}.conf
    - contents: "blacklist {{ module }}"
      {% endfor %}
    {% endif %}
  {% endfor %}
{% endif %}

compute_net_modules:
  kmod.present:
    - mods:
      - bridge
      - br_netfilter
    - persist: True

net.bridge.bridge-nf-call-ip6tables:
  sysctl.present:
    - value: 1
    - require:
      - kmod: compute_net_modules

net.bridge.bridge-nf-call-iptables:
  sysctl.present:
    - value: 1
    - require:
      - kmod: compute_net_modules

/etc/yc/compute-node/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/config.yaml
    - require:
      - yc_pkg: yc-compute-node

{%- if grains['cluster_map']['environment'] == 'dev' and grains['cluster_map']['stand_type'] == 'virtual' %}
enable-node:
  cmd.run:
    - name: /usr/bin/yc-compute-admin nodes enable --timeout 300 {{ grains['nodename'] }}
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8
    - require:
      - service: yc-compute-node
{%- endif %}

/var/lib/yc/compute-node/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/files/system_account.json
    - mode: '0040'
    - replace: False
    - user: root
    - group: yc-compute-node
    - require:
      - yc_pkg: yc-compute-node
