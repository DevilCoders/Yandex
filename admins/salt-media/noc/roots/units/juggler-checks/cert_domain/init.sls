/etc/monrun/salt_cert_domain/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - config-monrun-cert-check

/etc/monrun/salt_cert_domain/cert_domain.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cert_domain.sh
    - makedirs: True
    - mode: 755
