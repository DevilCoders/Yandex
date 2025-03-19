{% import slspath ~ "/init.jinja" as defaults %}


apparmor_packages:
  yc_pkg.installed:
    - pkgs:
      - apparmor
      - apparmor-utils

yc_apparmor_policies_files_dir:
  file.directory:
    - name: {{ defaults.policies_files_dir }}
    - mode: 760
    - user: root
    - group: root
    - require:
      - yc_pkg: apparmor_packages

yc_apparmor_tunables_dir:
  file.directory:
    - name: {{ defaults.tunables_dir }}
    - mode: 760
    - user: root
    - group: root
    - require:
      - file: yc_apparmor_policies_files_dir

yc_apparmor_abstractions_dir:
  file.directory:
    - name: {{ defaults.abstractions_dir }}
    - mode: 760
    - user: root
    - group: root
    - require:
      - file: yc_apparmor_policies_files_dir

apparmor_service:
  service.running:
    - name: apparmor
    - enable: True
    - reload: True
    - force: True
    - require:
      - yc_pkg: apparmor_packages
      - file: yc_apparmor_policies_files_dir
      - file: yc_apparmor_tunables_dir
      - file: yc_apparmor_abstractions_dir

apparmor_set_default_action:
  cmd.run:
    - name: |
        set -xe
        cd /etc/apparmor.d
        find ./ -mindepth 1 -maxdepth 1 \! -type d -printf '%f\n' | grep -v '^qemu$\|^lxc-containers$' | while read -r profile; do
            'aa-{{ pillar["apparmor"]["default"] }}' "$profile"
        done
    - require:
      - yc_pkg: apparmor_packages
      - service: apparmor_service

apparmor_setup:
  test.nop:
    - require:
      - service: apparmor_service
      - cmd: apparmor_set_default_action
