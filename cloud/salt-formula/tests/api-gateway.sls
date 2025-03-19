yc-api-gateway-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-api-gateway-tests

/etc/yc/api-gateway-tests/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/api-gateway-tests-config.yaml
    - require:
      - yc_pkg: yc-api-gateway-tests
