include:
  - units.pkgver_ignore

/usr/sbin/jmonrun:
  file.managed:
    - source: salt://{{ slspath }}/files/jmonrun.py
    - mode: 755
  pkg.installed:
    - pkgs:
      - python3-tabulate
      - python3-termcolor
