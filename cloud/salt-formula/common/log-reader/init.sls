log-reader-package:
  yc_pkg.installed:
    - pkgs:
      - yc-log-reader

/etc/yc/log-reader/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - require:
      - yc_pkg: log-reader-package

/etc/yc/log-reader/secret.txt:
  file.managed:
    - source: salt://{{ slspath }}/files/secret.txt
    - replace: false
    - user: yc-log-reader
    - mode: 600
    - require:
      - yc_pkg: log-reader-package

/etc/logrotate.d/log-reader:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate.conf
    - require:
      - yc_pkg: log-reader-package

yc-log-reader:
  service.running:
    - enable: true
    - watch:
      - file: /etc/yc/log-reader/config.yaml
      - yc_pkg: log-reader-package
