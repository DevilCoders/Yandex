Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/migrator/log4j2.yaml:
        - source: salt://application/log4j2.yaml
      - /usr/local/bin/init_docker.sh:
        - source: salt://application/init_docker.sh
        - mode: '555'
      - /usr/local/bin/start_migration.sh:
        - source: salt://application/start_migration.sh
        - mode: '555'

/usr/local/bin/start_migration.sh:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/start_migration.sh
    - context:
        migrator_application_version: {{ grains['migrator_application_version'] }}

/etc/kubelet.d/app.yaml:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/pod.yaml
    - context:
        migrator_application_version: {{ grains['migrator_application_version'] }}

