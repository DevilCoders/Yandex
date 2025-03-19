yc-healthcheck-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-healthcheck-tests


/etc/yc/healthcheck-tests/config.yaml:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/healthcheck-tests-config.yaml
    - require:
      - yc_pkg: yc-healthcheck-tests
