import-docker-key:
  cmd.run:
    - name: curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
    - creates: /etc/apt/sources.list.d/docker.list

/etc/apt/sources.list.d/docker.list:
  file.managed:
    - source: salt://docker.list

docker:
  pkg.installed:
    - pkgs:
      - docker-ce
      - docker-ce-cli
      - containerd.io
    - refresh: True
    - require:
      - file: /etc/apt/sources.list.d/docker.list
  service.running:
    - name: docker
    - require:
      - pkg: docker
