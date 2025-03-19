htop:
  yc_pkg.installed:
    - pkgs:
      - htop
  file.managed:
    - name: /etc/htoprc
    - source: salt://{{ slspath }}/files/htoprc
    - require:
      - yc_pkg: htop