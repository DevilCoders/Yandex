/usr/local/share/teamcity:
  file.directory:
    - user: teamcity
    - group: dpt_virtual_robots
    - dir_mode: 755
    - makedirs: True

mercurial:
  pkg.installed

{% set user=salt.environ.get('LC_USER') or 'teamcity' %}
ssh://{{user}}@hg.yandex-team.ru/media/yandex-build:
    hg.latest:
      - target: /usr/local/share/teamcity/yandex-build
      - require:
        - file: /usr/local/share/teamcity
      - require_in:
        - cmd: chown_yandex-build

chown_yandex-build:
  cmd.run:
    - name: chown -R teamcity:dpt_virtual_robots /usr/local/share/teamcity/yandex-build

/var/log/yandex-build-update.log:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots

/usr/bin/hg pull -u -R /usr/local/share/teamcity/yandex-build &> /var/log/yandex-build-update.log:
  cron.present:
    - user: teamcity
    - minute: '*/15'
