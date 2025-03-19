{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}
{% set mtu = grains['cluster_map']['underlay']['mtu'] %}

include:
  - common.network_interfaces

{{ sls }}-install_package:
  yc_pkg.installed:
    - pkgs:
      - python-netconfig

{{ sls }}-sysctl:
  file.managed:
    - name: /etc/sysctl.d/99-ipvs_tun.conf
    - mode: '0664'
    - source: salt://{{ slspath }}/files/99-ipvs_tun.conf
  cmd.wait:
    - name: "sysctl -p /etc/sysctl.d/99-ipvs_tun.conf"
    - watch:
      - file: {{ sls }}-sysctl

{{ sls }}-kmod:
  kmod.present:
    - mods:
      - dummy
      - ipip
      - ip6_tunnel
      - tunnel6
    - persist: True

python-netconfig:
  yc_pkg.installed:
    - pkgs:
      - python-netconfig

{{ sls }}-netconfig_plugin:
  file.managed:
    - name: /etc/netconfig.d/01-disable-{{ underlay_interfaces.management }}-config.py
    - source: salt://{{ slspath }}/files/netconfig_plugin.py
    - template: jinja
    - defaults:
        iface: {{ underlay_interfaces.management }}
    - require:
      - yc_pkg: {{ sls }}-install_package

# We run this at network reconfiguration stage. This means no interface changes
# such as eth0 -> vhost0 are allowed. Not a problem, as we don't deploy SLB on
# compute nodes.
{{ sls }}-gen_tun_interfaces:
  file.copy:
    - name: {{ prefix }}/network/interfaces.d/11_yc_slb
    - source: /etc/network/interfaces.d/11_yc_slb
    - preserve: True
    - require:
      - file: network_config_init
    - onlyif:
      # For some reason, file.copy fails if source is absent
      - test -e /etc/network/interfaces.d/11_yc_slb
  filex.from_command:
    - name: {{ prefix }}/network/interfaces.d/11_yc_slb
    - cmd: netconfig --iface={{ underlay_interfaces.management }} --mtu={{ mtu }}
    - retries: 9
    - retry_backoff: 20
    - require:
      - yc_pkg: {{ sls }}-install_package
      - file: {{ sls }}-netconfig_plugin
      # We compare against the current effective config which is really in /etc.
      # So we need to replicate it into our "safe location" first.
      - file: {{ sls }}-gen_tun_interfaces
      - cmd: {{ sls }}-sysctl

{{ sls }}-sync_tun_interfaces:
  file.copy:
    - name: /etc/network/interfaces.d/11_yc_slb
    - source: {{ prefix }}/network/interfaces.d/11_yc_slb
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

extend:
  networking_stopped:
    service:
      - onchanges:
        - filex: {{ prefix }}/network/interfaces.d/11_yc_slb
  networking_started:
    service:
      - require:
        - file: /etc/network/interfaces.d/11_yc_slb
