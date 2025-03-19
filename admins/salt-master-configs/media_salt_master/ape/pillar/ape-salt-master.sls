{% set is_prod = grains['yandex-environment'] == 'production' %}

salt_master:
  lookup:
    config: salt://ape-master.conf
    git_local:
        - storage-secure:
            git: git@github.yandex-team.ru:salt-media/storage-secure.git
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
        - storage:
            git: git@github.yandex-team.ru:salt-media/storage.git
