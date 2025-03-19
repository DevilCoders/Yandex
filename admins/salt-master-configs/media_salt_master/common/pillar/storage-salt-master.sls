{%- set is_prod = grains['yandex-environment'] == 'production' %}
{%- set project = salt['grains.get']('conductor:project', 'storage') %}
{%- if project == 'ape' and is_prod %}
{%-   set project = 'ape_prod' %}
{%- endif %}

salt_master:
  lookup:
    config: salt://templates/storage-master/master.conf
    git_local: []
    packages:
      - yandex-salt-components

salt_minion:
  lookup:
    params:
      saltenv: master
      pillarenv_from_saltenv: True
    packages:
      - yandex-salt-components

dynamic_roots:
    enabled: True
    user: robot-media-salt
    config:
        git_mirrors_dir: '/srv/salt/mirrors'
        git_checkouts_dir: '/srv/salt/checkouts'
        repos:
            storage:
                url: https://github.yandex-team.ru/salt-media/storage.git
                branch: ~
            storage_secure:
                url: ssh://git@github.yandex-team.ru/salt-media/storage-secure.git
                branch: master
            salt_media_common:
                url: https://github.yandex-team.ru/salt-media/common.git
                branch: master
        file_roots_envs_repo: storage
        pillar_roots_envs_repo: storage
        file_roots:
            - 'storage_secure:{master}/{{project}}/roots'
            - 'storage_secure:{master}/common/roots'
            {%- if project == 'elliptics' %}
            - 'storage_secure:{master}/storage/roots/'
            - '{_ENV_}/storage/roots'
            {%- endif %}
            - '{_ENV_}/{{project}}/roots'
            - '{_ENV_}/common/roots'
            - 'salt_media_common:{master}/roots'
        pillar_roots:
            - 'storage_secure:{master}/{{project}}/pillar'
            - 'storage_secure:{master}/common/pillar'
            {%- if project == 'elliptics' %}
            - 'storage_secure:{master}/storage/pillar'
            - '{_ENV_}/storage/pillar'
            {%- endif %}
            - '{_ENV_}/{{project}}/pillar'
            - '{_ENV_}/common/pillar'
            - 'salt_media_common:{master}/pillar'
        file_roots_config_path: /srv/salt/configs/dynamic_roots_config.yaml
        pillar_roots_config_path: /srv/salt/configs/dynamic_pillar_config.yaml
    cron: '* * * * * robot-media-salt'
    log_file: /var/log/salt/dynamic_roots.log
