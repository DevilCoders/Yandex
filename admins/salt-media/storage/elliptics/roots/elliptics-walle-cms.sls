include:
  - templates.certificates

/etc/yandex/mds_cms/config.json:
  file.managed:
    - makedirs: True
    - mode: 640
    - owner: www-data
    - group: www-data
    - source: salt://files/walle_cms/config.json
    - template: jinja

/etc/yandex/mds_cms/schema.json:
  file.managed:
    - makedirs: True
    - mode: 640
    - owner: www-data
    - group: www-data
    - source: salt://files/walle_cms/schema.json
    - template: jinja

walle-automation-enabled:
  file.managed:
    - name: /usr/local/bin/monrun_automation_enabled.sh
    - mode: 755
    - user: root
    - group: root
    - source: salt://files/walle_cms/monrun_automation_enabled.sh
  monrun.present:
    - command: /usr/local/bin/monrun_automation_enabled.sh
    - execution_interval: 300
    - execution_timeout: 60

/srv/mds_cms/cms/robot.id_rsa:
  file.managed:
    - mode: 640
    - user: www-data
    - group: www-data
    - content: {{ pillar.get("walle_ssh_key", "No key provided, please fix") | json }}
