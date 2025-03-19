{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' %}
  {% set branch = 'stable' %}
{% else %}
  {%set branch = 'testing' %}
{% endif %}

salt_master:
  lookup:
    config: 'salt://master.conf-{{ yaenv }}'
    csync2:
      secdist_path: '/repo/projects/weather/csync/sport-{{ yaenv }}'
    git_local:
      - common:
          git: git@github.yandex-team.ru:salt-media/common.git
          branch: master
      - sport:
          git: git@github.yandex-team.ru:sport/salt-sport.git
          branch: {{ branch }}
    ssh:
      key_path: '/repo/projects/weather/robots/robot-weather-admin/ssh'
      key_name: 'robot-weather-admin'
    params:
      user: robot-weather-admin
