{{ sls }}-pkgs:
  pkg.installed:
    - pkgs:
      - git
      - python
      - python-dev
      - python-virtualenv
      - python-pip
      - libssl-dev
      - libffi-dev

{{ sls }}-git:
  git.latest:
    - name: https://github.yandex-team.ru/YandexCloud/automated-tests.git
    - target: /root/automated-tests

{{ sls }}-pip:
  pip.installed:
    - name: virtualenv

{{ sls }}-work_dir:
  virtualenv.managed:
    - name: /root/automated-tests
    - pip_exists_action: s
    - pip_pkgs:
      - requests
      - configparser
      - pytest
      - joblib
      - bs4
      - paramiko
      - decorator
      - pytest-xdist
      - pytest-timeout
      - pytest-ignore-flaky
      - pytest-pylint
      - pytest-allure-adaptor
      - execnet==1.1
      - xunitmerge
      - py
      - futures
      - simpledist
      - jsonschema

{{ sls }}-pip_install_execnet:
  pip.installed:
    - cwd: /root/automated-tests
    - name: execnet == 1.1
    - bin_env: /root/automated-tests

{{ sls }}-pip_install_yc-snapshot-client:
  pip.installed:
    - cwd: /root/automated-tests
    - name: yc-snapshot-client
    - bin_env: /root/automated-tests
    - editable: https://pypi.yandex-team.ru/simple/
