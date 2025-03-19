/etc/monrun/salt_coredump/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - yandex-coredump-monitoring

/etc/monrun/salt_coredump/coredump.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/coredump.sh
    - makedirs: True
    - mode: 755
