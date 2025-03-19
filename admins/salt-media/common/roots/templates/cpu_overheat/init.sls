{% set unit = 'cpu_overheat' %}
{% set check_path = pillar.get('cpu_overheat_check_path') %}
{% if not check_path %}
{% set check_path = pillar.get('cpu_overheat_sh_path', "/usr/local/bin/cpu_overheat.sh") %}
{% endif %}

{{ check_path }}:
  file.managed:
    - source: salt://templates/cpu_overheat/cpu_overheat.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

cpu_overheat:
  monrun.present:
    - command: "{{ check_path }}"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage
