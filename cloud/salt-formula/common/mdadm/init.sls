mdadm:
  yc_pkg.installed:
    - pkgs:
      - mdadm

/etc/cron.d/mdadm:
  file.managed:
    - source: salt://{{ slspath }}/files/mdadm
    - require:
      - yc_pkg: mdadm
