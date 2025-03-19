Docker config:
  file.managed:
    - makedirs: True
    - names:
        # docker config for build
        - /home/ubuntu/.docker/config.json:
            - source: salt://docker/docker-config.json
        # docker config for users
        - /etc/skel/.docker/config.json:
            - source: salt://docker/docker-config.json

Pull docker image:
  cmd.run:
    - name: docker pull {{ grains['app_image'] }}:{{ grains['app_version'] }}
