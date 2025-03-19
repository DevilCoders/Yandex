apparmor packages:
  pkg.installed:
    - pkgs:
      - apparmor
      - apparmor-utils

apparmor service:
  service.running:
    - name: apparmor
    - enable: True
    - reload: True

add apparmor to kernel bootcmd:
  file.append:
    - name: /etc/default/grub
    - text: |
        # Load apparmor LSM -- CLOUD-20946
        GRUB_CMDLINE_LINUX_DEFAULT="$GRUB_CMDLINE_LINUX_DEFAULT apparmor=1 security=apparmor"

update grub:
  cmd.wait:
    - name: update-grub
    - watch:
      - file: /etc/default/grub
