Application Configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/yc-iot/tank/environment:
        - source: salt://application/environment

/usr/local/bin/init_docker.sh:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/init_docker.sh
    - mode: '555'
    - context:
        tank_application_version: {{ grains['tank_application_version'] }}

/usr/local/bin/shoot:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/shoot
    - mode: '555'
    - context:
        tank_application_version: {{ grains['tank_application_version'] }}

/usr/local/bin/tank:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://application/tank
    - mode: '555'
    - context:
        tank_application_version: {{ grains['tank_application_version'] }}
