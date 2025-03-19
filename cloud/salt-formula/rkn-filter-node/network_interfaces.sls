{% set downstream_ipv4_address = grains['cluster_map']['hosts'][grains['nodename']]['ipv4']['addr']%}

configure_eth_interfaces:
    file.managed:
      - name: /etc/network/interfaces.d/20_rfn_eth_interfaces
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/network/interfaces.d/20_rfn_eth_interfaces.j2
      - defaults: {{  salt['netinfra_interfaces_management.generate_interface_ip'](downstream_ipv4_address) }}
      - template: jinja
      - makedirs: True


configure_loopback_interfaces:
   file.managed:
     - name: /etc/network/interfaces.d/20_rfn_lo_interfaces
     - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/network/interfaces.d/20_rfn_lo_interfaces.j2
     - template: jinja
     - makedirs: True


configure_gre_interfaces:
    file.managed:
      - name: /etc/network/interfaces.d/30_rfn_gre_interfaces
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/network/interfaces.d/30_rfn_gre_interfaces.j2
      - template: jinja
      - makedirs: True


remove_original_eth1_conf_file: ## may be deleted after cloud-19998 implementation
    file.absent:
      - name: /etc/network/interfaces.d/10_yc_base_ipv4_cfg


restart_eth_interfaces:
    cmd.run:
      - names:
          - ifdown eth1
          - ip addr flush dev eth1
          - ifup eth1
      - onchanges:
          - file: configure_eth_interfaces


restart_loopback_interfaces:
    cmd.run:
      - names: 
          - ifdown lo:666
          - ip addr flush dev lo
          - ip addr add 127.0.0.1 dev lo
          - ip addr add ::1/128 dev lo
          - ifup lo:666
      - onchanges:
          - file: configure_loopback_interfaces


restart_gre_interfaces:
    cmd.run:
      - names:
          - ifdown gre_to_STD
          - ifup gre_to_STD
          - ifdown gre_to_M9
          - ifup gre_to_M9
      - onchanges:
        - file: configure_gre_interfaces

disabling_rpf_persistently:
  sysctl.present:
    - name: net.ipv4.conf.default.rp_filter  
      value: 2

disabling_rpf:
  sysctl.present:
    - name: net.ipv4.conf.all.rp_filter  
      value: 2

enable_kernel_routing:
  sysctl.present:
    - name: net.ipv4.ip_forward 
      value: 1