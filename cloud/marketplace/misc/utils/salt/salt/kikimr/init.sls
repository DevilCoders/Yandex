/etc/yc-marketplace/populate-kikimr-config.yaml:
  file.managed:
    - source: salt://kikimr/files/config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

populate:
  cmd.run:
    - name: sudo docker login -u {{ pillar['common']['docker_user'] }} -p {{ pillar['security']['docker_token'] }} registry.yandex.net && docker pull {{ pillar['common']['docker_api'] }} && docker tag {{ pillar['common']['docker_api'] }} marketplace