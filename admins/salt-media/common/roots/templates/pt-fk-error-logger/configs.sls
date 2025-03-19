/etc/ubic/service/pt-fk-error-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic
    - makedirs: True

/etc/yandex/percona-toolkit/pt-fk-error-logger/config:
  file.managed:
    - source: salt://{{ slspath }}/files/config
    - makedirs: True

/etc/init.d/pt-fk-error-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/init
    - makedirs: True
    - mode: 755

/etc/logrotate.d/pt-fk-error-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate
    - makedirs: True
