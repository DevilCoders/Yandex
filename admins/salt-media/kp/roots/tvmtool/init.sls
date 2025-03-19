yandex-passport-tvmtool:
  pkg.installed

/etc/tvmtool/tvmtool.conf:
  yafile.managed:
    - source: salt://tvmtool/tvmtool.conf
    - template: jinja
    - mode: 0644
    - require:
      - pkg: yandex-passport-tvmtool
    - watch_in:
      - cmd: restart_tvmtool

restart_tvmtool:
  cmd.wait:
    - name: service yandex-passport-tvmtool restart
