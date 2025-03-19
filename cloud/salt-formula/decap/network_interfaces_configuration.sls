{% set downstream_ipv4_address = grains['cluster_map']['hosts'][grains['nodename']]['ipv4']['addr']%}


disable_dad_for_kni1_interface:
  sysctl.present:
    - name: net.ipv6.conf.kni1.dad_transmits 
      value: 0


configure_kni_interfaces:
    file.managed:
      - name: /etc/network/interfaces.d/20_decap_interfaces
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/network/interfaces.d/20_decap_interfaces.j2
      - defaults: {{  salt['netinfra_interfaces_management.generate_interface_ip'](downstream_ipv4_address) }}
      - template: jinja
      - makedirs: True


remove_original_eth1_conf_file: ## may be deleted after cloud-19998 implementation
    file.absent:
      - name: /etc/network/interfaces.d/10_yc_base_ipv4_cfg


flush_interfaces:
    cmd.run:
      - names:
          - ip addr flush dev eth1
          - ip addr flush dev eth2
      - onchanges:
          - file: configure_kni_interfaces
          - sysctl: disable_dad_for_kni1_interface


restart_kni_interfaces:
    cmd.run:
      - names:
          - ifdown kni0
          - ifdown kni1
          - ip addr flush dev kni0
          - ip addr flush dev kni1
          - ifup kni0
          - ifup kni1
      - onchanges:
          - file: configure_kni_interfaces
          - sysctl: disable_dad_for_kni1_interface