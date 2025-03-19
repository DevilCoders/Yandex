{%- set hostname = grains['nodename'] %}
{%- set host_config = grains['cluster_map']['hosts'][hostname] %}
{%- set ipv4_pool = host_config.get('ipv4_pool', []) %}

{%- set underlay = grains['cluster_map']['underlay'] %}

/usr/local/bin/config-vf:
  file.managed:
    - source: salt://{{ slspath }}/files/config-vf
    - mode: 0755
    - user: root
    - group: root
    - follow_symlinks: False
    - require:
      - file: /etc/yc/yc-network-profiles.json

{# Remove after CLOUD-19998 release #}
/usr/local/bin/spasibo-krasj:
  file.absent

# Here we need iproute 2 some least version
# to make some ip commands in our 
# configure-underlay service
iproute2:
  yc_pkg.installed:
    - pkgs:
      - iproute2

/etc/yc/yc-network-profiles.json:
  file.serialize:
    - dataset:
        underlay-v4:
          vlan:  {{ underlay.ipv4_vlan }}
          require_ipv4_address: true
        underlay-v6:
          vlan: {{ underlay.ipv6_vlan }}
        upstream-v4:
          vlan: {{ underlay.upstream_ipv4_vlan }}
        upstream-v6:
          vlan: {{ underlay.upstream_ipv6_vlan }}
        V01D:
          vlan: 999
          trust: "off"
    - formatter: json
    - makedirs: True

/etc/yc/yc-underlay-ipv4-pool.json:
  file.serialize:
    - dataset: {{ ipv4_pool|yaml }}
    - formatter: json
    - makedirs: True

{%- set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}
configure-underlay:
  service.running:
    {#
      NOTE(k-zaitsev): Disable this server on SVMs, since it's part of a cycle with cloud-init.
      A better fix should remove these files from svms. For now just disabling.
    #}
    - enable: {{ 'True' if base_role == 'cloudvm' or grains['virtual'] == 'physical' else 'False' }}
    - require:
      - file: /etc/systemd/system/configure-underlay.service
      - file: /usr/local/bin/config-vf
      - file: /etc/yc/yc-network-profiles.json
      - file: /etc/yc/yc-underlay-ipv4-pool.json
      - yc_pkg: iproute2
{#
  Note(k-zaitsev): We're deliberately not adding any watch'es here
  Re-configuring underlay VFs on the fly prooved to be rather dangerous
  and error-prone. Instead this it should be handled manually or with migrations.
  See CLOUD-11705 for more details.
#}

/etc/systemd/system/configure-underlay.service:
  file.managed:
    - source: salt://{{ slspath }}/files/configure-underlay.service.j2
    - template: jinja
    - require:
      - file: /usr/local/bin/configure_underlay_devices.py

/usr/local/bin/configure_underlay_devices.py:
  file.managed:
    - source: salt://{{ slspath }}/files/configure_underlay_devices.py
    - mode: 0755

{%- if grains['virtual'] == 'physical' %}
vfio-pci-module:
  kmod.present:
    - name: vfio_pci
    - persist: True
    - require_in:
      - service: configure-underlay

{# NOTE (k-zaitsev): Set sriov_numvfs==sriov_totalvfs for PF with zero VFs.
For Intel 710 cards set 127 explicitly. This is due to alleged bug in ip link.
It crashes when accessing 127th (zero-based idx) VF. CLOUD-5861 #}
/etc/udev/rules.d/sriov-vfs.rules:
  file.managed:
    - contents: |
        SUBSYSTEM=="pci", ATTR{sriov_numvfs}=="0", TEST=="/sys/$devpath/net/eth0", ATTR{vendor}=="0x8086", ATTR{device}=="0x1584", ATTR{sriov_numvfs}="127"
        SUBSYSTEM=="pci", ATTR{sriov_numvfs}=="0", TEST=="/sys/$devpath/net/eth0", ATTR{sriov_numvfs}="$attr{sriov_totalvfs}"

sriov-trigger-udev:
  cmd.run:
    - names:
      - /sbin/udevadm control --reload-rules
      - /sbin/udevadm trigger -a sriov_numvfs=0
      - /sbin/udevadm settle
      # NOTE(k-zaitsev): Make sure the udev in initrd has these files
      - /usr/sbin/update-initramfs -u
    - onchanges:
      - file: /etc/udev/rules.d/sriov-vfs.rules
    - require_in:
      - service: configure-underlay

tune-host-networking:
  service.running:
    - enable: True
    - require:
      - file: /etc/systemd/system/tune-host-networking.service
      - service: configure-underlay
    - watch:
      - file: /etc/systemd/system/tune-host-networking.service
      - file: /usr/local/bin/tune_host_networking.py
      - file: /etc/yc/tune-host-networking.yaml

/etc/systemd/system/tune-host-networking.service:
  file.managed:
    - source: salt://{{ slspath }}/files/tune-host-networking.service.j2
    - template: jinja
    - require:
      - file: /usr/local/bin/tune_host_networking.py

/usr/local/bin/tune_host_networking.py:
  file.managed:
    - source: salt://{{ slspath }}/files/tune_host_networking.py
    - mode: 0755

/etc/yc/tune-host-networking.yaml:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/tune-host-networking.yaml
{%- endif %}
