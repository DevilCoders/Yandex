{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set vhost_dev = pillar['network']['interfaces']['vhost_dev'] %}
{% set stand_type = grains['cluster_map']['stand_type'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}


# Install bare minimum OpenContrail packages to set up vhost0

# TODO: ifup/down script for vhost0 interface need scripts
# from package contrail-vrouter-agent.
# So. For now WE HAVE to install here not only dkms but also full
# contrail-vrouter-agent and configure scripts in it with /etc/contrail/agent_param
#
# While this network configuration is done in compute role, we can't really
# divide roles on network and compute.

# And to prevent restart on first upgrade we have to patch package here as well!
# NOTE(@xelez): remove after vrouter-agent package is upgraded to >= 3.2.3.80
patch_contrail_vrouter_agent_prerm_2:
  file.replace:
    - name: /var/lib/dpkg/info/contrail-vrouter-agent.prerm
    - pattern: ^(\s*deb-systemd-invoke\s+stop.*)
    - repl: 'true; #\1'
    - ignore_if_missing: True

vrouter_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-vrouter-dkms
      - contrail-vrouter-utils
      - contrail-vrouter-agent
      - contrail-vrouter-agent-dbg
    - hold_pinned_pkgs: True
    - require:
      - file: patch_contrail_vrouter_agent_prerm_2

/etc/sysctl.d/90-vrouter.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/90-vrouter.conf

/etc/udev/rules.d/90-vrouter.rules:
  file.managed:
    - source: salt://{{ slspath }}/files/90-vrouter.rules
    - require:
      - file: /etc/sysctl.d/90-vrouter.conf
    - require_in:
      - yc_pkg: vrouter_packages

/etc/udev/rules.d/vhost-net.rules:
  file.managed:
    - contents: 'KERNEL=="vhost-net", GROUP="kvm", MODE="0660"'

compute_net_modules:
  kmod.present:
    - mods:
      - vhost_net
      - bridge
      - br_netfilter
    - persist: True
    - require:
      - file: /etc/udev/rules.d/vhost-net.rules

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

{%- if stand_type == 'virtual' %}
{% set vrouter_module_params = 'vr_flow_entries=1600 vr_oflow_entries=1600 vr_bridge_entries=1600 vr_bridge_oentries=1600' %}
{% else %}
{% set vrouter_module_params = 'vr_max_flow_queue_entries=48' %}
{% endif %}

/etc/modprobe.d/vrouter.conf:
  file.managed:
    - contents: "options vrouter {{ vrouter_module_params }}"
    - require_in:
      - yc_pkg: vrouter_packages

# Configure vhost0 interface

/etc/contrail/agent_param:
  file.managed:
    - source: salt://{{ slspath }}/files/agent_param
    - template: jinja
    - defaults:
        vhost_dev: {{ vhost_dev }}
        vhost_inner_dev: {{ underlay_interfaces.dataplane }}
    - require:
      - yc_pkg: vrouter_packages

{{ prefix }}/network/if-pre-up.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_pre_up
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: vrouter_packages

/etc/network/if-pre-up.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-pre-up.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - service: networking_stopped

{{ prefix }}/network/if-up.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_up
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: vrouter_packages

/etc/network/if-up.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-up.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - service: networking_stopped
      - yc_pkg: vrouter_packages

{{ prefix }}/network/if-down.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_down
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: vrouter_packages

/etc/network/if-down.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-down.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - service: networking_stopped

{{ prefix }}/network/interfaces.d/09_vhost_underlay:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/09_vhost_underlay
    - template: jinja
    - source: salt://{{ slspath }}/files/09_vhost_underlay
    - defaults:
        inet_dev: {{ underlay_interfaces.dataplane }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/09_vhost_underlay:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/09_vhost_underlay
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - service: networking_stopped

extend:
{% if grains['virtual'] != 'physical' %}
  {{ prefix }}/network/interfaces.d/10_yc_base_cfg:
    file.managed:
      # Overrride network.head values with compute-specific
      - context:
         inet6_dev: {{ underlay_interfaces.management }}
{% endif %}
  {{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg:
    file.managed:
      # Overrride network.head values with compute-specific
      - context:
         inet_dev: {{ vhost_dev }}
  networking_stopped:
    service:
      - require:
        # Add this so if we failed to obtain packages, we don't
        # stop networking and don't loose the box.
        - yc_pkg: vrouter_packages
        - kmod: compute_net_modules
      - onchanges:
        - file: {{ prefix }}/network/if-pre-up.d/vhost
        - file: {{ prefix }}/network/if-up.d/vhost
        - file: {{ prefix }}/network/if-down.d/vhost
        - file: {{ prefix }}/network/interfaces.d/09_vhost_underlay
        - file: /etc/contrail/agent_param
  networking_started:
    service:
      - require:
        - file: /etc/network/if-pre-up.d/vhost
        - file: /etc/network/if-up.d/vhost
        - file: /etc/network/if-down.d/vhost
        - file: /etc/network/interfaces.d/09_vhost_underlay
