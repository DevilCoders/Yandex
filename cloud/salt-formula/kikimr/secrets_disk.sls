{% set dm_path = "/dev/disk/by-id/dm-name-" ~ dm_name %}
{% set password_file = cfg_dir + "/password" %}
{% set mount_unit = mount_where[1:]|replace("/", "-") %}


secrets_disk_pkgreqs:
  yc_pkg.installed:
    - pkgs:
      - cryptsetup-bin
      - udev

secrets_disk_cfg_dir:
  file.directory:
    - makedirs: true
    - name: {{ cfg_dir }}
    - user: root
    - group: root
    - mode: '0711'

secrets_disk_password_file:
  file.managed:
    - name: {{ password_file }}
    - replace: False
    - user: root
    - group: root
    - mode: '0640'
    - require:
      - file: secrets_disk_cfg_dir

secrets_disk_udev_rules:
  service.running:
    - name: udev
    - reload: True
  file.managed:
    - name: /etc/udev/rules.d/100-kikimr.secrets_disk.rules
    - user: root
    - group: root
    - mode: '0600'
    - source: salt://{{ slspath }}/files/secrets_disk/udev.rules.tmpl
    - template: jinja
    - defaults:
        device_serial: {{ device_serial }}
        dm_name: {{ dm_name }}
        password_file: {{ password_file }}
    - require:
      - yc_pkg: secrets_disk_pkgreqs
      - file: secrets_disk_password_file
      - service: secrets_disk_udev_rules

secrets_disk_udevd_service:
  service.running:
    - name: systemd-udevd

secrets_disk_dm_dev_closed:
  service.dead:
    - name: {{ mount_unit }}.automount
  cmd.run:
    - name: "{ ! [ -b '{{ dm_path }}' ]; } || cryptsetup close '{{ dm_name }}'"
    - require:
      - service: secrets_disk_dm_dev_closed

secrets_disk_dm_dev_ready:
  cmd.run:
    - name: |
        while :; do
          for dev in /dev/vd*; do
            udevadm trigger -v --action=add --name-match "$dev"
          done
          [ -b '{{ dm_path }}' ] && udevadm info '{{ dm_path }}' | cut -d' ' -f2 | grep 'DM_NAME={{ dm_name }}' && break
          sleep 1
        done
    - timeout: 120
    - require:
      - secrets_disk_dm_dev_closed

secrets_disk_systemd_reload:
  module.wait:
    - name: service.systemctl_reload

secrets_disk_mount_unit:
  file.managed:
    - name: /etc/systemd/system/{{ mount_unit }}.mount
    - user: root
    - group: root
    - mode: '0640'
    - source: salt://{{ slspath }}/files/secrets_disk/secrets_disk.mount.tmpl
    - template: jinja
    - defaults:
        disk_label: "KIMIMR_SECRETS"
        target_dir: {{ mount_where }}
        owner_group: kikimr
    - require:
      - cmd: secrets_disk_dm_dev_ready
      - file: {{ mount_where }}

secrets_disk_automount_unit:
  file.managed:
    - name: /etc/systemd/system/{{ mount_unit }}.automount
    - user: root
    - group: root
    - mode: '0640'
    - source: salt://{{ slspath }}/files/secrets_disk/secrets_disk.automount.tmpl
    - template: jinja
    - defaults:
        target_dir: {{ mount_where }}
    - require:
      - file: {{ mount_where }}
    - watch_in:
      - cmd: secrets_disk_systemd_reload

secrets_disk_mount_unit_stopped:
  service.dead:
    - name: {{ mount_unit }}.mount
    - enable: false
    - watch:
      - file: secrets_disk_mount_unit
    - require:
      - file: /etc/systemd/system/{{ mount_unit }}.mount

secrets_disk_automount_unit_started:
  service.running:
    - name: {{ mount_unit }}.automount
    - restart: true
    - enable: true
    - watch:
      - file: secrets_disk_automount_unit
    - require:
      - secrets_disk_mount_unit_stopped
      - file: /etc/systemd/system/{{ mount_unit }}.automount
      - secrets_disk_mount_unit
