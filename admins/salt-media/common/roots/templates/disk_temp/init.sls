{% set unit = 'disk_temp' %}
{% set check_path = pillar.get('disk_temp_check_path') %}
{% if not check_path %}
{% set check_path = pillar.get('disk_temp_sh_path', "/usr/local/bin/disk_temp.sh") %}
{% endif %}

{{ check_path }}:
  file.managed:
    - source: salt://templates/disk_temp/disk_temp.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

disk_temp:
  monrun.present:
    - command: "/usr/bin/timeout 160 {{ check_path }}; if [ $? -eq 124 ];then echo '1; Timeout';fi"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
