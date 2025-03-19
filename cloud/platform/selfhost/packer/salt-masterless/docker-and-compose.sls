{% set teamcity_agent_image_name = 'jetbrains/teamcity-agent' %}
{% set teamcity_agent_image_tag = '2019.2.2' %}

docker dpkg dependencies:
  pkg.installed:
    - pkgs:
      - apt-transport-https
      - ca-certificates
      - curl
      - software-properties-common

docker packages:
  pkg.latest:
    - name: docker-ce
    - skip_verify: False

docker config:
  file.managed:
    - name: /etc/docker/daemon.json
    - source: salt://docker-and-compose/docker-daemon-config.json
    - mode: 644
    - user: root

docker legacy config:
  file.absent:
    - name: /etc/default/docker

add docker cert for cr.gpn.yandexcloud.net:
  file.managed:
    - name: /etc/docker/certs.d/cr.gpn.yandexcloud.net/GPNInternalRootCA.crt
    - source: salt://docker-and-compose/GPNInternalRootCA.crt
    - mode: 644
    - user: root
    - makedirs: True

docker service:
  service.running:
    - name: docker
    - enable: True

install docker-compose:
  file.managed:
    - name: /usr/bin/docker-compose
    - source: https://github.com/docker/compose/releases/download/1.24.0/docker-compose-Linux-x86_64
    - source_hash: bee6460f96339d5d978bb63d17943f773e1a140242dfa6c941d5e020a302c91b
    - mode: 755

install compose yaml config:
  file.managed:
    - name: /etc/compose/docker-compose.yml
    - source: salt://docker-and-compose/docker-compose.yml
    - template: jinja
    - makedirs: True
    - context:
        image: {{ teamcity_agent_image_name }}
        tag: {{ teamcity_agent_image_tag }}

and additional files:
  file.managed:
    - name: /etc/compose/run-masquerade.sh
    - source: salt://docker-and-compose/run-masquerade.sh
    - mode: 755

and additional files 2:
  file.managed:
    - name: /etc/compose/run-ssh.sh
    - source: salt://docker-and-compose/run-ssh.sh
    - mode: 755

and additional files 3:
  file.managed:
    - name: /etc/compose/add-agent-properties.sh
    - source: salt://docker-and-compose/add-agent-properties.sh
    - mode: 755

docker-compose service:
  file.managed:
    - name: /lib/systemd/system/docker-compose.service
    - source: salt://docker-and-compose/docker-compose.service
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: /lib/systemd/system/docker-compose.service

docker-compose service is running:
  service.dead:
    - name: docker-compose
    - enable: True

'docker pull {{ teamcity_agent_image_name }}:{{ teamcity_agent_image_tag }}':
  cmd.run
