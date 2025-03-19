{% set virtual_host_flag = salt['pillar.get']('virtual_host_flag', '/etc/yandex/virtual_host') %}

{% if grains['virtual'] == 'physical' %}

{{ virtual_host_flag }}:
  file.absent

{% else %}

{{ virtual_host_flag }}:
  file.managed:
    - user: root
    - mode: 644
    - contents:
      - {{ grains['virtual'] }}

/etc/monitoring/unispace.conf:
  file.managed:
    - source: salt://files/virtual-host/etc/monitoring/unispace.conf
    - makedirs: True
    - user: root
    - mode: 644

{% endif%}
