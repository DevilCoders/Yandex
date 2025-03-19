/etc/docker/daemon.json:
  file.managed:
    - source: salt://configs/docker/daemon.json

docker:
  service.running:
    - name: docker.service
    - enable: True
