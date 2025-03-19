{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set interfaces = salt['underlay.interfaces']() %}
{% set underlay_ipv6 = salt['network.ip_addrs6'](interfaces.management) %}
{#
  32 bytes margin should be enough for virtually every sane
  method of encapsulating overlay network packets.
  Currently used methods are:
    * MPLS over UDP (4 + 8 bytes)
    * MPLS over GRE (4 + 4..20 bytes)

  576 is a minimum MTU for IPv4 networks. See RFC791 for details

  ([value1, value2]|sort)[-1] is a workaround for jinja lacking max function
#}
{% set overlay_mtu = ([salt['grains.get']('cluster_map:underlay:mtu', 8950) - 32, 576]|sort)[-1] %}

include:
  - common.dns.selfdns
  - slb

{{ prefix }}/network/interfaces.d/10_yc_overlay_cfg:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/10_yc_overlay_cfg
    - template: jinja
    - source: salt://{{ slspath }}/files/10_yc_overlay_cfg
    - defaults:
        overlay_dev: {{ interfaces.overlay }}
        overlay_mtu: {{ overlay_mtu }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/10_yc_overlay_cfg:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/10_yc_overlay_cfg
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

disable_overlay_ipv6_default_route:
  sysctl.present:
    - name: net.ipv6.conf.{{ interfaces.overlay }}.accept_ra_defrtr
    - value: 0
    - config: /etc/sysctl.d/90-overlay-ra-defrtr.conf

disable_underlay_dad:
  sysctl.present:
    - name: net.ipv6.conf.{{ interfaces.management }}.accept_dad
    - value: 0
    - config: /etc/sysctl.d/90-underlay-dad.conf

# Underlay IPv6 in/etc/hosts
{% for ipv6 in underlay_ipv6 %}
underlay-ipv6-in-etc-hosts-{{ loop.index }}:
  host.present:
    - names:
      - {{ salt['grains.get']('nodename') }}
      - {{ salt['grains.get']('host') }}
    - ip: {{ ipv6 }}
    - onchanges:
      - service: networking_started
{% endfor %}

extend:
  networking_stopped:
    service:
      - onchanges:
        - file: {{ prefix }}/network/interfaces.d/10_yc_overlay_cfg
  networking_started:
    service:
      - require:
        - file: /etc/network/interfaces.d/10_yc_overlay_cfg
