yc-cli:
  yc_pkg.installed:
    - pkgs:
      - yc-cli

/etc/profile.d/python-requests-use-system-CA.sh:
  yc_pkg.installed:
    - pkgs:
      - ca-certificates
  file.managed:
    - contents:
      - export REQUESTS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt

/etc/yc/cli/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/config.yaml
    - require:
      - yc_pkg: yc-cli
