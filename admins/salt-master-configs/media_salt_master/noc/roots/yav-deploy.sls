{% set robot = salt['pillar.get']('master:user') %}
{% from "/srv/salt/secrets/secrets.sls" import robot_yav_token with context %}

/etc/yandex/yav-deploy/pkg/salt-master/defaults.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://files/yav-deploy/defaults.conf
    - template: jinja
    - context:
        salt_user: {{ robot }}

/etc/yandex/yav-deploy/pkg/salt-master/templates/yav.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://files/yav-deploy/yav.conf

# generates /etc/salt/master.d/yav.conf file
# will fail if you logged in as root instead of sudo
yav-deploy /etc/salt/master.d/yav.conf:
  cmd.run:
    - name: /usr/bin/yav-deploy --debug
    - env:
      - YAV_TOKEN: {{robot_yav_token}}
