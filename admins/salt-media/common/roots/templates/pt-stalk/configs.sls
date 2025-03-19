/etc/ubic/service/pt-stalk:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic
    - makedirs: True

/etc/yandex/percona-toolkit/pt-stalk/config:
  file.managed:
    - source: salt://{{ slspath }}/files/config
    - makedirs: True
    - template: jinja

/etc/yandex/percona-toolkit/pt-stalk/plugin:
  file.managed:
    - source: salt://{{ slspath }}/files/plugin
    - makedirs: True
    - template: jinja

/etc/init.d/pt-stalk:
  file.managed:
    - source: salt://{{ slspath }}/files/init
    - makedirs: True
    - mode: 755

/etc/logrotate.d/pt-kill:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate
    - makedirs: True
