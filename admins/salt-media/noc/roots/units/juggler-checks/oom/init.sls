/etc/monrun/salt_oom/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - yandex-media-common-oom-check

/etc/monrun/salt_oom/oom.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/oom.sh
    - makedirs: True
    - mode: 755
