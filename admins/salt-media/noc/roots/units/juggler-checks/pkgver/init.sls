/etc/monrun/salt_pkgver/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-monitoring-pkgver

/etc/monrun/salt_pkgver/pkgver.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/pkgver.sh
    - makedirs: True
    - mode: 755
