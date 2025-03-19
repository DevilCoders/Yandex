salt_minion:
  lookup:
    params:
      saltenv: master
      pillarenv_from_saltenv: True
    packages:
      - yandex-salt-components
