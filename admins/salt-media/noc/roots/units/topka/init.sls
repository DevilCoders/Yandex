{% set env = grains["yandex-environment"] %}

/etc/nginx/sites-enabled/30-topka.conf:
  file.managed:
    - source: salt://{{slspath}}/files/etc/nginx/sites-enabled/30-topka.conf-{{env}}
    - makedirs: True

/etc/topka/config.yaml:
  file.managed:
    - source: salt://{{slspath}}/files/etc/topka/config.yaml
    - makedirs: True
    - template: jinja

topka-rbt:
  user.present:
    - createhome: True
    - system: True
    - home: /home/topka-rbt
    - groups:
      - root
/home/topka-rbt/.ssh/id_rsa:
  file.managed:
    - user: topka-rbt
    - group: root
    - mode: 600
    - makedirs: True
    - contents_pillar: 'sec:robot_ssh_private'
/home/topka-rbt/.ssh/config:
  file.managed:
    - user: topka-rbt
    - group: topka-rbt
    - mode: 600
    - makedirs: True
    - contents: |
        Host tree.yandex.ru
            User robot-topka-test
