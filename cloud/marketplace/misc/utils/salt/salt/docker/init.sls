common:
  cmd.run:
    - name: sudo docker login -u {{ pillar['common']['docker_user'] }} -p {{ pillar['security']['docker_token'] }} registry.yandex.net && sudo service docker start
