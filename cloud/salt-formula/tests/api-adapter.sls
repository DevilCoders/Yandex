yc-api-adapter-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-api-adapter-tests

/etc/yc/api-adapter-tests/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/api-adapter-tests-config.yaml
    - require:
      - yc_pkg: yc-api-adapter-tests
