yc-automated-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-automated-tests

/etc/yc/automated-tests/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/automated-tests-config.yaml
    - require:
      - yc_pkg: yc-automated-tests
