/etc/monrun/salt_cert_internal/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-monrun-cert-check

/etc/monrun/salt_cert_internal/cert_internal.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cert_internal.sh
    - makedirs: True
    - mode: 755
