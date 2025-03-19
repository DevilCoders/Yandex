/etc/docker/daemon.json:
  file.managed:
    - source: salt://configs/docker/daemon.json

/lib/systemd/system/docker.service:
  file.managed:
    - source: salt://services/docker.service
