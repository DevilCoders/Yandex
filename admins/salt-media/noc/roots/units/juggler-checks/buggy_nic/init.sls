/etc/monrun/salt_buggy_nic/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-autodetect-active-eth

/etc/monrun/salt_buggy_nic/buggy_nic.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/buggy_nic.sh
    - makedirs: True
    - mode: 755
