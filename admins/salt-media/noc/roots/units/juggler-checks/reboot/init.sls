/etc/monrun/salt_reboot/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-monrun-reboot-count

/etc/monrun/salt_reboot/reboot.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/reboot.sh
    - makedirs: True
    - mode: 755
