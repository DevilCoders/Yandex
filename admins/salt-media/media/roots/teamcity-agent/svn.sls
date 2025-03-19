subversion:
  pkg.installed

# pull arcadia yatool with current user
{% set user=salt.environ.get('LC_USER') or 'teamcity' %}
svn+ssh://{{user}}@arcadia-ro.yandex.ru/arc/trunk/arcadia/devtools/ya:
  svn.latest:
    - target: /home/teamcity/yatool

chown -R teamcity:dpt_virtual_robots /home/teamcity/yatool:
  cmd.run

/var/log/ya-tool-update.log:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots

/usr/bin/svn update /home/teamcity/yatool &> /var/log/ya-tool-update.log:
  cron.present:
    - user: teamcity
    - minute: '*/15'

# CADMIN-8471, KPDUTY-2032
/home/teamcity/.subversion/config:
  file.absent

