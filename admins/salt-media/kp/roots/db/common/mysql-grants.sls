# grants config
/etc/mysql-configurator/grants.yaml:
  yafile.managed:
    - source: salt://{{slspath}}/files/mysql-grants.yaml
    - makedirs: True

/etc/mysql-configurator/privileges.yaml:
  file.managed:
    - mode: 0600
    - makedirs: True
    - contents: {{ pillar['privileges'] | json }}
