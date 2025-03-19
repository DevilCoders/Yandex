yc-billing-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-billing-tests

/etc/yc/billing-tests/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/billing-tests-config.yaml
    - require:
      - yc_pkg: yc-billing-tests
