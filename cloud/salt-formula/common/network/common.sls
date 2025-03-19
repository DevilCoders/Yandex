{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}

include:
  - common.network_devices

yc-network-config-pkg:
  yc_pkg.installed:
    - pkgs:
      - yc-network-config
{%- if grains['virtual'] == 'physical' %}
    - disable_update: True
{% endif %}

{{ prefix }}/network/interfaces:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/interfaces
    - require:
      - file: network_config_init

# cloud-init also configures the network (usually with DHCP)
# if present. Prevent it from doing so with this piece of
# configuration.
/etc/cloud/cloud.cfg.d/99_yc_network.cfg:
  file.managed:
    - source: salt://{{ slspath }}/files/99_yc_network.cfg
    - onlyif:
      - test -d /etc/cloud/cloud.cfg.d

# TODO(k-zaitsev): Moving from /var/run to /var/lib. Remove after CLOUD-10651 has been deployed
fix_network_prefix:
  file.directory:
    - name: /var/lib/yc
    - user: root
    - group: root
    - dir_mode: 755
  cmd.run:
    - onlyif: test -d {{ pillar['network']['old_tmp_config_prefix'] }}
    - name: mv {{ pillar['network']['old_tmp_config_prefix'] }} {{ prefix }}
    - require:
      - file: fix_network_prefix

# Prepare network configuration skeleton in the safe location.
# Every state which renders a piece of network configuration
# must require this state.
#
# The reason is we shouldn't change currently running configuration
# before stopping it, or there will be leftovers. And we can't
# render new configuration while the network is stopped, as file.maanged
# requires roundtrips to Salt master to fetch templates.
network_config_init:
  file.directory:
    - names:
      - {{ prefix }}/network/if-down.d
      - {{ prefix }}/network/if-post-down.d
      - {{ prefix }}/network/if-pre-up.d
      - {{ prefix }}/network/if-up.d
      - {{ prefix }}/network/interfaces.d
    - makedirs: True
    - require:
      - cmd: fix_network_prefix

{% if grains['virtual'] != 'physical' %}
{{ prefix }}/network/interfaces.d/10_yc_base_cfg:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/10_yc_base_cfg
    - template: jinja
    - source: salt://{{ slspath }}/files/10_yc_base_cfg
    - defaults:
        inet6_dev: {{ underlay_interfaces.management }}
    - require:
      - service: configure-underlay
      - file: network_config_init

/etc/network/interfaces.d/10_yc_base_cfg:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/10_yc_base_cfg
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean
{% endif %}

{%- set stand_type = grains['cluster_map']['stand_type'] %}
{%- if stand_type == 'virtual' %}
# Static default routes to workaround obscure Contrail bugs.
# See CLOUD-4331.
# We want this in virtual stands only, as keeping it generic
# introduces new bugs and is somewhat costly (e.g. CLOUD-4419).
{{ prefix }}/network/interfaces.d/12_yc_routing_hack:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/12_yc_routing_hack
    - template: jinja
    - source: salt://{{ slspath }}/files/12_yc_routing_hack
    - defaults:
        inet6_dev: {{ underlay_interfaces.management }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/12_yc_routing_hack:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/12_yc_routing_hack
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean
{%- else %}
# Remove it where deployed
{{ prefix }}/network/interfaces.d/12_yc_routing_hack:
  file.absent:
    - require:
      - file: network_config_init

/etc/network/interfaces.d/12_yc_routing_hack:
  file.absent:
    - require:
      - file: interfaces_d_clean
{%- endif %}


# Stop networking before we start dismounting the current config.
# Network roles (such as head and compute), SLB and opencontrail.control
# extend this state with '- onchange: config snippet', so it only runs when
# something was really changed.
networking_stopped:
  service.dead:
    - name: networking
    # Make sure we rendered what we wanted to render ourselves
    - onchanges:
      - file: {{ prefix }}/network/interfaces
      - file: {{ prefix }}/network/interfaces.d/12_yc_routing_hack
{% if grains['virtual'] != 'physical' %}
      - file: {{ prefix }}/network/interfaces.d/10_yc_base_cfg
{% endif %}
      - yc_pkg: yc-network-config-pkg
      - file: /etc/cloud/cloud.cfg.d/99_yc_network.cfg

# Delete everything in interfaces.d: we own it now.
#
# But don't blindly remove files we manage: they are
# deleted or copied over as needed in the respective states
# such as network.head or opencontrail.control.
#
# Otherwise we'll get strange side effects such as applying
# opencontrail.control state alone wipes SLB configs due to
# greedy inetrfaces_d_clean.
interfaces_d_clean:
  file.directory:
    - name: /etc/network/interfaces.d
    - clean: True
    - exclude_pat: '*_yc_*'
    # Clean /etc/network/interfaces.d ONLY if networking was stopped
    - onchanges:
      - service: networking_stopped

/etc/network/interfaces:
  file.copy:
    - source: {{ prefix }}/network/interfaces
    - preserve: True
    # Overwrite even if already present
    - force: True
    # OK to copy this file every time highstate runs. But
    # see below on require vs watch for networking_started.
    - require:
      - file: interfaces_d_clean

# Copy the new configuration from its temporary location
# into /etc where it really belongs.
# This happens only after (and when) networking is stopped.
#
# Other states which render network configuration snippets
# should copy them into /etc after networking_stopped and
# 'require' themseves in here with 'extend: service'.
#
# Start networking after we finished configureation.
networking_started:
  service.running:
    - name: networking
    - enable: True
    # It's OK to run this state regardless was stopped or not
    # In the latter case it would just be a no-op. But DON'T
    # add '- watch: ' here as we don't want networking restarted.
    - require:
      - file: /etc/network/interfaces
      - file: /etc/network/interfaces.d/12_yc_routing_hack
{% if grains['virtual'] != 'physical' %}
      - file: /etc/network/interfaces.d/10_yc_base_cfg
{% endif %}
      - yc_pkg: yc-network-config-pkg
