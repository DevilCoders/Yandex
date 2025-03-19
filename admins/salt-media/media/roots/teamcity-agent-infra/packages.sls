juggler_fix_broken:
  pkg.purged:
    - pkgs:
      - config-juggler-search
      - juggler-client
    - onlyif:
      - dpkg -l | grep config-juggler-search

juggler_client:
  pkg.installed:
    - name: juggler-client
    - version: 2.2.09261506
    - watch:
      - juggler_fix_broken

common_packages:
  pkg.installed:
    - pkgs:
      - git
      - subversion
      - config-caching-dns
      - config-yabs-ntp
      - yandex-timetail
      - config-monitoring-common
      - config-juggler-client-media
      - config-monrun-monitoring-alive
      - config-monrun-reboot-count
      - config-monrun-drop-check
      - yandex-media-common-oom-check
      - yandex-coredump-monitoring
      - unzip
      - s3cmd

python-pip:
  pkg.installed:
    - pkgs:
      - python-pip
      - python3-pip

Uptodate_pip:
  pip.installed:
    - bin_env: '/usr/bin/pip'
    - name: pip
    - upgrade: True
    - require:
      - pkg: python-pip

Uptodate_pip3:
  pip.installed:
    - name: pip
    - upgrade: True
    - bin_env: '/usr/bin/pip3'
    - require:
      - pkg: python-pip

Uptodate_setuptools:
  pip.installed:
    - name: setuptools
    - upgrade: True
    - require:
      - pkg: python-pip

Uptodate_setuptools3:
  pip.installed:
    - bin_env: '/usr/bin/pip3'
    - name: setuptools
    - upgrade: True
    - require:
      - pkg: python-pip

releaser-cli[all]:
  pip.installed:
    - index_url: https://pypi.yandex-team.ru/simple/
    - require:
      - pkg: python-pip

pip3_packages:
  pip.installed:
    - bin_env: '/usr/bin/pip3'
    - index_url: https://pypi.yandex-team.ru/simple/
    - names:
      - PyYAML
      - GitPython
      - startrek_client
      - st-release-task
      - deployer_cli
      - sandbox-library
      - docker-compose
      - setuptools_rust
      - bitbucket-server-client
    - require:
      - pkg: python-pip

pip_packages:
  pip.installed:
    - bin_env: '/usr/bin/pip'
    - index_url: https://pypi.yandex-team.ru/simple/
    - names:
      - ammo
    - require:
      - pkg: python-pip

yarn:
  pkgrepo.managed:
    - humanname: "Yarn repo"
    - name: deb https://dl.yarnpkg.com/debian/ stable main
    - file: /etc/apt/sources.list.d/yarn.list
    - keyid: 23E7166788B63E1E
    - keyserver: keyserver.ubuntu.com

  pkg.latest:
    - name: yarn
    - refresh: true
