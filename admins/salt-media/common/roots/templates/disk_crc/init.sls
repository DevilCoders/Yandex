{% set unit = 'disk_crc' %}
{% set check_path = pillar.get('disk_crc_check_path') %}
{% if not check_path %}
{% set check_path = pillar.get('disk_crc_sh_path', "/usr/local/bin/disk_crc.sh") %}
{% endif %}

{{ check_path }}:
  file.managed:
    - source: salt://templates/disk_crc/disk_crc_check.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/etc/cron.yandex/disk-crc.sh:
  file.managed:
    - source: salt://templates/disk_crc/disk_crc_cron.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/etc/cron.d/disk-crc:
  file.managed:
    - source: salt://templates/disk_crc/disk-crc
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

disk_crc:
  monrun.present:
    - command: {{ check_path }}
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
