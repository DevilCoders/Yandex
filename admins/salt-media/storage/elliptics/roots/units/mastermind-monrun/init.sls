{% set unit = 'mastermind-monrun' %}
{%- set federation = pillar.get('mds_federation', None) -%}

/usr/bin/mastermind-monrun.py:
  file.managed:
    - source: salt://{{ slspath }}/files/mastermind-monrun.py
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/mastermind.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/mastermind.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja

mastermind-inventory:
  monrun.present:
    - command: if [ -e /var/tmp/inventory ]; then echo '2; /var/tmp/inventory exist'; else echo '0; Ok'; fi
    - execution_interval: 300
    - execution_timeout: 30
    - type: mastermind

monrun_inventory:
  monrun.present:
    - command: /usr/bin/monrun_inventory.sh
    - execution_interval: 900
    - execution_timeout: 800
    - type: mastermind

  file.managed:
    - name: /usr/bin/monrun_inventory.sh
    - source: salt://{{ slspath }}/files/monrun_inventory.sh
    - user: root
    - group: root
    - mode: 755

long_jobs:
  file.managed:
    - name: /usr/bin/mongo_long.py
    - source: salt://{{ slspath }}/files/mongo_long.py
    - user: root
    - group: root
    - mode: 755

  monrun.present:
    - command: "/usr/bin/mongo_long.py 30"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage

future_backends:
  file.managed:
    - name: /usr/bin/future_backends.py
    - source: salt://{{ slspath }}/files/future_backends.py
    - user: root
    - group: root
    - mode: 755

  monrun.present:
    - command: "/usr/bin/future_backends.py --monrun"
    - execution_interval: 1800
    - execution_timeout: 300
    - type: storage

{% if grains['yandex-environment'] == 'production' and federation == 1 %}
/etc/cron.d/mastermind_monrun:
  file.managed:
    - contents: |
        0 14 * * 1-5 root zk-flock -n 1 move_from_1g "mastermind-monrun.py --run couples_on_1g_host" | grep -v "move_from_1g" | /bin/bash
        0 15 * * 1-5 root zk-flock -n 1 zk_man_x2 "mastermind-monrun.py --run man_x2" | grep -v "zk_man_x2" | /bin/bash
        0 16 * * 1-5 root zk-flock -n 1 zk_man_first_group "mastermind-monrun.py --run man_first_group" | grep -v "zk_man_first_group" | /bin/bash
    - user: root
    - group: root
    - mode: 755
/etc/cron.d/move_from_1g:
  file.absent
{% endif %}

{% for f in ['/etc/monrun/conf.d/couple_lost.conf',
            '/usr/bin/mm_couple-lost.py', 
            '/etc/monrun/conf.d/ns-settings-correct.conf',
            '/usr/local/bin/ns_correct.py'] %}
{{ f }}:
  file.absent:
    - name: {{ f }}
{% endfor %}
