install-apt-repositories:
   cmd.run:
      - name: |
            apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ xenial main' && \
            apt-add-repository -y ppa:ubuntu-toolchain-r/test

install-common-packages:
  pkg.installed:
    - pkgs:
      - 'apt-transport-https'
      - 'ca-certificates'
      - 'gnupg'
      - 'software-properties-common'
      - 'wget'
      - 'git'
      - 'gdebi-core'
      - 'python-pip'
    - failhard: True

install-build-packages:
  pkg.installed:
    - pkgs:
      - 'unixodbc'
      - 'build-essential'
      - 'cmake'
      - 'g++-7'
      - 'libpoco-dev'
      - 'libssl-dev'
      - 'libicu-dev'
      - 'unixodbc-dev'

install-pyodbc-pip:
  pip.installed:
    - name: pyodbc
