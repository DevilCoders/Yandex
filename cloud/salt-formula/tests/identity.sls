yc-identity-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-identity-tests

/etc/yc/identity-tests/config.toml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/identity-tests-config.toml
    - require:
      - yc_pkg: yc-identity-tests
