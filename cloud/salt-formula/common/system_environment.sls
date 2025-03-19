{%- set environment = grains['cluster_map']['environment'] %}

/etc/environment:
  file.managed:
    - source: salt://common/environment.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 0644

/etc/security/limits.d/core_limits.conf:
  file.managed:
    - source: salt://common/core_limits.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 0644

/var/log/journal:
  file.directory:
    - name: /var/log/journal
  cmd.run:
    - names:
      - systemd-tmpfiles --create --prefix /var/log/journal
    - require: 
      - file: /var/log/journal
    - onchanges:
      - file: /var/log/journal
    - require_in:
      - file: /etc/systemd/journald.conf.d/yc-defaults.conf

/etc/systemd/journald.conf.d/yc-defaults.conf:
  file.managed:
    - source: salt://common/journald.conf
    - template: jinja
    - makedirs: True

reload_config_journald:
  cmd.run:
    - names:
      - journalctl --flush
      - systemctl restart systemd-journald.service
    - onchanges:
      - file: /etc/systemd/journald.conf.d/yc-defaults.conf

systemd-journald-audit.socket:
  service.masked

systemd-journald.service:
  service.running:
    - watch :
       - service: systemd-journald-audit.socket

/etc/profile.d/debconf_dash:
  file.managed:
    - contents:
      - 'dash dash/sh boolean false'

/etc/debian_chroot:
  file.managed:
    - contents:
      - '{{ environment | upper }}'

update_bashrc_root:
  file.managed:
    - name: '/root/.bashrc'
    - source: salt://common/bashrc_skel
    - user: root
    - group: root
    - mode: 0644

update_bashrc_skel:
  file.managed:
    - name: '/etc/skel/.bashrc'
    - source: salt://common/bashrc_skel
    - user: root
    - group: root
    - mode: 0644

{{ sls }}-change_shell:
  cmd.run:
    - name: debconf-set-selections /etc/profile.d/debconf_dash && dpkg-reconfigure -f noninteractive dash
    - unless: 
      - debconf-show dash|grep false
      - readlink -f /bin/sh|grep bash
    - require:
      - file: /etc/profile.d/debconf_dash
