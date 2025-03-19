{% set cgroup = grains['conductor']['group'] %}
{% if 'ape-' in cgroup %}
{% set prj = 'ape' %}
{% else %}
{% set prj = 'NULL' %}
{% endif %}
{% if 'front' in cgroup %}
{% set front = true %}
{% else %}
{% set front = false %}
{% endif %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pidenv = 'production' %}
{% else %}
{% set pidenv = 'testing' %}
{% endif %}


/etc/network/projectid:
  file.managed:
    - source: salt://units/netconfiguration/files/etc/network/projectid.{{ prj }}.{{ pidenv }}
    - user: root
    - mode: 644

/etc/network/interfaces:
  file.managed:
    - source: salt://units/netconfiguration/files/etc/network/interfaces.simple
    - user: root
    - mode: 644

{% if front %}
/etc/rc.conf.local:
  file.managed:
    - source: salt://units/netconfiguration/files/etc/rc.conf.local
    - user: root
    - mode: 644
{% endif %}
