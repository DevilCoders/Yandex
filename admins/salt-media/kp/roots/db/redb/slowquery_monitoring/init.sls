percona-toolkit_pkg:
  pkg.installed:
    - pkgs:
      - percona-toolkit

/var/log/slowquery:
  file.directory:
    - user: root
    - group: root
    - mode: 755

/usr/bin/slowquery_check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/slowquery_check.sh
    - user: root
    - group: root
    - mode: 755

/usr/bin/slowquerystats.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/slowquerystats.sh
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/slowquery.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/slowquery.conf
    - user: root
    - group: root
    - mode: 644

/etc/logrotate.d/slowquery:
  file.managed:
    - source: salt://{{ slspath }}/files/slowquery.logrotate
    - user: root
    - group: root
    - mode: 644
