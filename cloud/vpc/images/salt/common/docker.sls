import-docker-key:
  cmd.run:
    - name: curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
    - creates: /etc/apt/sources.list.d/docker.list

/etc/apt/sources.list.d/docker.list:
  file.managed:
    - source: salt://{{ slspath }}/files/docker/docker.list

/etc/docker/daemon.json:
  file.managed:
    - source: salt://{{ slspath }}/files/docker/daemon.json
    - require:
      - pkg: docker

docker:
  pkg.installed:
    - pkgs:
      - docker-ce
      - docker-ce-cli
      - containerd.io
      - python3-docker  # for salt integration with docker
  service.running:
    - name: docker
    - enable: true
    - require:
      - pkg: docker
    - watch:
      - file: /etc/docker/daemon.json
