/etc/ubic/service/pt-deadlock-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic
    - makedirs: True

/etc/yandex/percona-toolkit/pt-deadlock-logger/config:
  file.managed:
    - source: salt://{{ slspath }}/files/config
    - makedirs: True

/etc/init.d/pt-deadlock-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/init
    - makedirs: True
    - mode: 755

/etc/logrotate.d/pt-deadlock-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate
    - makedirs: True
