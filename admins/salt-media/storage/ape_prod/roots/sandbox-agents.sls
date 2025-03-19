/etc/apt/sources.list.d/searchkernel.list:
  file.managed:
    - source: "salt://sandbox-agents/etc/apt/sources.list.d/searchkernel.list"
    - makedirs: True
    - mode: 644
    - user: root
    - group: root

/etc/apt/sources.list.d/porto.list:
  file.managed:
    - source: "salt://sandbox-agents/etc/apt/sources.list.d/porto.list"
    - makedirs: True
    - mode: 644
    - user: root
    - group: root

/etc/apt/sources.list.d/docker-repo-config.list:
  file.managed:
    - source: "salt://sandbox-agents/etc/apt/sources.list.d/docker-repo-config.list"
    - makedirs: True
    - mode: 644
    - user: root
    - group: root

kernel:
  pkg.installed:
    - refresh: True
    - pkgs:
      - linux-image-3.18.32-32
    - skip_suggestions: True
    - install_recommends: False

sandbox:
  group.present:
    - name: sandbox
  user.present:
    - shell:
    - home: /home/sandbox
    - uid: 134303
    - gid: sandbox

docker:
  group.present:
    - system: True
    - members:
      - sandbox
  pkg.installed:
    - pkgs:
      - docker-engine
    - skip_suggestions: True
    - install_recommends: False

porto:
  group.present:
    - system: True
    - members:
      - sandbox
  pkg.installed:
    - refresh: True
    - pkgs:
      - yandex-portoinit
    - skip_suggestions: False
    - install_recommends: True

/etc/default/docker:
  file.managed:
    - source: "salt://sandbox-agents/etc/default/docker"
    - makedirs: True
    - mode: 6000
    - user: root
    - group: root

skynet:
  pkg.installed:
    - pkgs:
      - yandex-gosky
    - skip_suggestions: False
    - install_recommends: True

vcs:
  pkg.installed:
    - pkgs:
      - subversion
      - git
    - skip_suggestions: False
    - install_recommends: True

/etc/sudoers.d/sandbox_manual:
  file.managed:
    - source: "salt://sandbox-agents/etc/sudoers.d/sandbox_manual"
    - makedirs: True
    - mode: 440
    - user: root
    - group: root
