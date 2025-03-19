# used by nocdev-staging.sls
/etc/mysync.yaml:
  file.managed:
    - source: salt://units/mysql/files/etc/mysync.yaml
    - template: jinja
