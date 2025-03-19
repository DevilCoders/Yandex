python-packages:
  pkg.installed:
    - pkgs:
      - python3
      - python3-yaml

/usr/sbin/remove_old_coredumps:
  file.managed:
    - source: salt://{{ slspath }}/remove_old_coredumps
    - user: root
    - group: root
    - mode: 755
    - require:
      - pkg: python-packages

/etc/remove_old_coredumps.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        config:
          directory: /home/cores/Desktop/cores
          keep: 10

/etc/cron.d/remove_old_coredumps:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /usr/sbin/remove_old_coredumps
      - file: /etc/remove_old_coredumps.conf
    - contents: |
        1 1 * * 6  root /usr/sbin/remove_old_coredumps
