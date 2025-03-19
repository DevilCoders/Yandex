{% set downstream_ipv4_address = grains['cluster_map']['hosts'][grains['nodename']]['ipv4']['addr']%}


modify_controlplane_configuration_file:
    file.managed:
      - name: /etc/yadecap/controlplane.conf
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/yadecap/controlplane.conf.j2
      - defaults: {{  salt['netinfra_interfaces_management.generate_interface_ip'](downstream_ipv4_address) }}
      - template: jinja
      - makedirs: True


restart_yadecap_controlplane:
    service.running:
      - name: yadecap-controlplane
      - enable: True
      - watch:
          - file: modify_controlplane_configuration_file