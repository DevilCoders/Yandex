/etc/monrun/salt_cert_expires/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
    - template: jinja
  pkg.installed:
    - pkgs:
      - config-monrun-cert-check

/etc/monrun/salt_cert_expires/cert_expires.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cert_expires.sh
    - makedirs: True
    - mode: 755
