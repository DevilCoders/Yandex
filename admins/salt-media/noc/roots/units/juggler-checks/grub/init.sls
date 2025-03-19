/etc/monrun/salt_grub/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - yandex-media-common-grub-check

/etc/monrun/salt_grub/grub.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/grub.sh
    - makedirs: True
    - mode: 755
