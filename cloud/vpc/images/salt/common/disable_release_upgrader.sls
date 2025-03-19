Remove packages:
  pkg.purged:
    - pkgs:
      - unattended-upgrades
      - ubuntu-release-upgrader-core
      - update-notifier-common

# chmod -x for all files in /etc/update-motd.d/
Disable motd update:
  file.directory:
    - name: /etc/update-motd.d/
    - user: root
    - group: root
    - dir_mode: 755
    - file_mode: 644
    - recurse:
      - mode
