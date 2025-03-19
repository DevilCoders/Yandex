include:
  - common

prepare-migrate:
  yc_pkg.installed:
    - pkgs:
      - yc-compute
  cmd.script:
    - source: salt://{{ slspath }}/files/prepare-migrate.sh
      template: jinja
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8
