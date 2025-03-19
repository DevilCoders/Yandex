common_packages:
  pkg.installed:
    - pkgs:
      - yandex-media-teamcity-agent
      - language-pack-ru-base
      - language-pack-ru
      - ant
      - percona-server-server-5.7
      - mongodb-org
      - libgeobase5-java
      - libgeobase6-java
      - wavegain
      - mp3check
      - imagemagick
      - yandex-video-codecs
      - python-mysqldb
      - jq
      - nscd
      - python3-virtualenv
      - vagrant
      # yandex-music-loader dependencies
      - id3v2
      - flac
      - optipng
      - mp3check
      - imagemagick
      - yandex-chromaprinter
      - yandex-music-mplayer-decoder: 11.24.3
      - xvfb
      - wavegain
      - maven3: 3.3.9-001+yandex0
      - yandex-jdk8
      - dh-systemd
      - fontconfig
      - brotli
      - python3.5
      - p2p-distribution-geodata6-config
      - libpq-dev
      - nodejs-5
      - yandex-openjdk14
      - yandex-fakeya
      - libuatraits
      - libuatraits-dev
      - libboost-dev

python-pip:
  pkg.installed:
    - pkgs:
      - python-pip
      - python3-pip

ant_packages:
  pkg.installed:
    - sources:
      - ant: http://mirrors.kernel.org/ubuntu/pool/universe/a/ant/ant_1.9.6-1ubuntu1_all.deb
      - ant-optional: http://mirrors.kernel.org/ubuntu/pool/universe/a/ant/ant-optional_1.9.6-1ubuntu1_all.deb

golang:
  cmd.run:
    - stateful: True
    - cwd: /tmp
    - name: |
        #!/bin/bash
        set -xe
        version=go1.10.2
        if grep -Fq "$version" /usr/local/go/VERSION; then
          echo "changed=no comment='Have actual version of golang: $version'"
        else
          echo "changed=yes comment='Install golang: $version'"
          archive=$version.linux-amd64.tar.gz
          wget --no-verbose https://storage.googleapis.com/golang/$archive
          tar xzf $archive --xform 's/^go/go.new/' -C /usr/local/
          mv -v -b -S .prev -T /usr/local/go.new /usr/local/go >&2
          rm -rf /usr/local/go.prev
        fi

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
      - yadi_bin
      - ats-mdb-patcher
      - sandbox-library
    - require:
      - pkg: python-pip

pip_packages:
  pip.installed:
    - bin_env: '/usr/local/bin/pip'
    - index_url: https://pypi.yandex-team.ru/simple/
    - names:
      - ammo
    - require:
      - pkg: python-pip

npm8_packages:
  cmd.run:
    - name: |
        /opt/nodejs/8/bin/npm install -g --registry http://npm.yandex-team.ru @yandex-int/qtools js-yaml yarn
        /opt/nodejs/8/bin/node /opt/nodejs/8/bin/npm install --global newman@4.6.1
        /opt/nodejs/8/bin/npm install -g --registry http://npm.yandex-team.ru npm-cache

sentry_cli:
  cmd.run:
    - name: curl -sL https://sentry.io/get-cli/ | bash
    - unless: sentry-cli --version

npm5_packages:
  cmd.run:
    - name: /opt/nodejs/5/bin/npm install -g --registry http://npm.yandex-team.ru npm-cache
    - require:
      - pkg: common_packages
