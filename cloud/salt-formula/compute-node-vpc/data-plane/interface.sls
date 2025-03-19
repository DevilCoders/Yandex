{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set vhost_dev = pillar['network']['interfaces']['vhost_dev'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}
{% set virtio_passthrough = salt['underlay.virtio_for_passthrough']() %}

include:
  - common.network_interfaces.base_ipv4
  - .packages

# TODO: ifup/down script for vhost0 interface need scripts
# from package contrail-vrouter-agent.

# Configure vhost0 interface

/etc/contrail/agent_param:
  file.managed:
    - source: salt://{{ slspath }}/files/agent_param
    - template: jinja
    - defaults:
        vhost_dev: {{ vhost_dev }}
        vhost_inner_dev: {{ underlay_interfaces.dataplane }}
    - require:
      - yc_pkg: contrail_vrouter_agent_packages

{{ prefix }}/network/if-pre-up.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_pre_up
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: contrail_vrouter_agent_packages

/etc/network/if-pre-up.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-pre-up.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

{{ prefix }}/network/if-up.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_up
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: contrail_vrouter_agent_packages

/etc/network/if-up.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-up.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

{{ prefix }}/network/if-down.d/vhost:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/vhost_down
    - mode: 0755
    - defaults:
        vhost_dev: {{ vhost_dev }}
    - require:
      - file: network_config_init
      - yc_pkg: contrail_vrouter_agent_packages

/etc/network/if-down.d/vhost:
  file.copy:
    - source: {{ prefix }}/network/if-down.d/vhost
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

{{ prefix }}/network/interfaces.d/09_yc_vhost_underlay:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/09_yc_vhost_underlay
    - template: jinja
    - source: salt://{{ slspath }}/files/09_yc_vhost_underlay
    - defaults:
        inet_dev: {{ underlay_interfaces.dataplane }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/09_yc_vhost_underlay:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/09_yc_vhost_underlay
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

# Set virtual underlay interface

{% if virtio_passthrough %}

{{ prefix }}/network/interfaces.d/20_yc_virtual_underlay:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/20_yc_virtual_underlay
    - template: jinja
    - source: salt://{{ slspath }}/files/20_yc_virtual_underlay
    - defaults:
        virtio_passthrough: {{ virtio_passthrough }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/20_yc_virtual_underlay:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/20_yc_virtual_underlay
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

{% else %}

no_need_virtual_underlay:
  file.absent:
    - names:
      - {{ prefix }}/network/interfaces.d/20_yc_virtual_underlay
      - /etc/network/interfaces.d/20_yc_virtual_underlay

{% endif %}


extend:
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
        - yc_pkg: contrail_vrouter_agent_packages
        - kmod: vrouter_net_modules
      - onchanges:
        - file: {{ prefix }}/network/if-pre-up.d/vhost
        - file: {{ prefix }}/network/if-up.d/vhost
        - file: {{ prefix }}/network/if-down.d/vhost
        - file: {{ prefix }}/network/interfaces.d/09_yc_vhost_underlay
        - file: {{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg
        - file: /etc/contrail/agent_param
        - file: {{ prefix }}/network/interfaces.d/20_yc_virtual_underlay
  networking_started:
    service:
      - require:
        - file: /etc/network/if-pre-up.d/vhost
        - file: /etc/network/if-up.d/vhost
        - file: /etc/network/if-down.d/vhost
        - file: /etc/network/interfaces.d/09_yc_vhost_underlay
        - file: /etc/network/interfaces.d/10_yc_base_ipv4_cfg
        - file: /etc/network/interfaces.d/20_yc_virtual_underlay
