{% set unit = 'disk_speed' %}
{% set check_path = pillar.get('disk_speed_check_path') %}
{% if not check_path %}
{% set check_path = pillar.get('disk_speed_sh_path', "/usr/local/bin/disk_speed.sh") %}
{% endif %}

{{ check_path }}:
  file.managed:
    - source: salt://templates/disk_speed/disk_speed.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

disk_speed:
  monrun.present:
    - command: "{{ check_path }}"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
