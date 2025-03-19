/etc/mysql/conf.d/custom.cnf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/mysql/conf.d/custom.cnf
    - template: jinja
