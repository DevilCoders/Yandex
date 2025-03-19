arc_pkgs:
  pkg.installed:
    - pkgs:
      - yandex-arc-launcher

/etc/fuse.conf:
  file.managed:
    - source: salt://{{ slspath }}/fuse.conf
    - user: root
    - group: fuse
    - mode: 640

fuse_group:
  cmd.run:
    - name: usermod -aG fuse teamcity
