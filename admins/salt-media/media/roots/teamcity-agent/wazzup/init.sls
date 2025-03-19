/usr/local/bin/wazzup:
  file.managed:
    - source: salt://{{ slspath }}/wazzup
    - user: root
    - group: root
    - mode: 755

/etc/xinetd.d/wazzup:
  file.managed:
    - source: salt://{{ slspath }}/wazzup.xinetd
    - user: root
    - group: root
    - mode: 644

xinetd-pkg:
  pkg.installed:
    - name: xinetd

xinetd:
  service.running:
    - enable: True
    - require:
        - file: /etc/xinetd.d/wazzup
        - pkg: xinetd-pkg
    - watch:
        - file: /etc/xinetd.d/wazzup
        - pkg: xinetd-pkg
