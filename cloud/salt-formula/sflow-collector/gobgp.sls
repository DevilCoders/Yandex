{% set gobgp_systemd_file_path = "files/etc/systemd/system/gobgp.service.j2" %}
{% set gobgp_configuration_file_path = "files/etc/gobgp/gobgpd.conf.j2" %}
{% set downstream_ipv4_address = grains['cluster_map']['hosts'][grains['nodename']]['ipv4']['addr']%}

install_gobgp:
  yc_pkg.installed:
    - pkgs:
      - gobgp

configure_gobgp_systemd_file:
  file.managed:
    - name: /etc/systemd/system/gobgp.service
    - source: salt://{{ slspath }}/{{ gobgp_systemd_file_path }}
    - template: jinja
    - makedirs: True
    - required:
      - yc_pkg: install_gobgp

configure_gobgp_daemon:
  file.managed:
    - name: /etc/gobgp/gobgpd.conf
    - source: salt://{{ slspath }}/{{ gobgp_configuration_file_path }}
    - defaults: {{  salt['netinfra_interfaces_management.generate_interface_ip'](downstream_ipv4_address) }}
    - template: jinja
    - makedirs: True
    - required:
      - yc_pkg: install_gobgp

apply_gobgp_systemd_configuration_changes:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: configure_gobgp_systemd_file

restart_gobgp_service:
  service.running:
    - name: gobgp
    - enable: True
    - watch:
      - file: configure_gobgp_systemd_file
      - file: configure_gobgp_daemon
      - yc_pkg: install_gobgp
