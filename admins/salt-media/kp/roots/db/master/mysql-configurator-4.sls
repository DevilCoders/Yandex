mysql_configurator_4_configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/mysql-configurator/backup.yaml:
        - source: salt://{{ slspath }}/files/mysql-configurator-4/backup.yaml
        - template: jinja
