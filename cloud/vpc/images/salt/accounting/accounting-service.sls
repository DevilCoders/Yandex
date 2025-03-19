yc-vpc-accounting:
  file.managed:
    - names:
      - /etc/systemd/system/yc-vpc-accounting.service:
        - source: salt://{{ slspath }}/files/yc-vpc-accounting.service
      - /etc/systemd/system/unified-agent.service:
        - source: salt://{{ slspath }}/files/unified-agent.service
  pkg.installed:
    - pkgs:
      - yc-vpc-accounting: 0.1-19812.220721
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: yc-vpc-accounting
  service.enabled:
    - watch:
      - module: yc-vpc-accounting

/var/log/yc/vpc-accounting:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

update_bashrc_root:
  file.managed:
    - name: '/root/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644

update_bashrc_skel:
  file.managed:
    - name: '/etc/skel/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644
