python-pip:
  pkg.installed

python3-pip:
  pkg.installed

pip_package:
  pip.installed:
    - pkgs:
      - filechunkio
      - boto
    - require:
      - pkg: python3-pip
    - bin_env: '/usr/bin/pip3'

