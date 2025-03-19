salt-minion:
  yc_pkg.installed:
    - pkgs:
      - salt-minion
      - salt-common
  service.dead:
    - enable: False
    - require:
      - yc_pkg: salt-minion
      - file: /etc/salt/minion.d/minion.conf
      - file: /etc/salt/minion.d/file_roots.conf
    - watch:
      - yc_pkg: salt-minion
      - file: /etc/salt/minion.d/minion.conf
      - file: /etc/salt/minion.d/file_roots.conf

/etc/salt/minion.d/minion.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/minion.conf
    - template: jinja
    - require:
      - yc_pkg: salt-minion

/etc/salt/minion.d/file_roots.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/file_roots.conf
    - template: jinja
    - require:
      - yc_pkg: salt-minion
