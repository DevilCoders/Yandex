{% set is_prod = grains['yandex-environment'] == 'production' %}
{% set sec_repo = 'music-secure' if is_prod else 'music-test-secure' %}

salt_master:
  lookup:
    csync2:
       repo: github.yandex-team.ru:salt-media/{{ sec_repo }}.git
       path: roots/salt/csync2
    ssh:
        key_repo: github.yandex-team.ru:salt-media/{{ sec_repo }}.git
        key_path_in_repo: roots/salt/robot-media-salt
        key_name_in_repo: id_rsa
    config: salt://master.conf
    git_local:
        {% if is_prod %}
        - {{ sec_repo }}:
            git: git@github.yandex-team.ru:salt-media/{{ sec_repo }}.git
        {% else %}
        - music-test-secure:
            git: git@github.yandex-team.ru:salt-media/music-test-secure.git
        {% endif %}
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
        - music:
            git: git@github.yandex-team.ru:salt-media/music.git
