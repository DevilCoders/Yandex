{% set downstream_ipv4_address = grains['cluster_map']['hosts'][grains['nodename']]['ipv4']['addr'] %}

sflow-anycast-loopback-configuration:
  file.managed:
    - name: /etc/network/interfaces.d/20_sflow_lo_interface
    - source: salt://{{ slspath }}/files/etc/network/interfaces.d/20_sflow_lo_interface.j2
    - template: jinja
    - makedirs: True

anycast-loopback-update:
  cmd.run:
    - names:
      - ifdown lo:555
      - ifup lo:555
    - onchanges:
      - file: sflow-anycast-loopback-configuration

upstream4:
  file.managed:
    - name: /etc/network/interfaces.d/20_upstream_interface
    - source: salt://{{ slspath }}/files/etc/network/interfaces.d/20_upstream_interface.j2
    - defaults: {{  salt['netinfra_interfaces_management.generate_interface_ip'](downstream_ipv4_address) }}
    - template: jinja
    - makedirs: True

upstream4-update:
  cmd.run:
    - names:
      - ifdown eth2
      - ip addr flush dev eth2
      - ifup eth2
    - onchanges:
      - file: /etc/network/interfaces.d/20_upstream_interface

enable_forwarding:
  sysctl.present:
    - name: net.ipv4.ip_forward
    - value: 1

disabling_rpf:
  sysctl.present:
    - name: net.ipv4.conf.all.rp_filter
      value: 2
