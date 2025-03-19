{% set cgroup = grains['conductor']['group'] %}
{% set dc = grains['conductor']['root_datacenter'] %}
{% set fqdn = grains['fqdn'] %}

# SEE https://st.yandex-team.ru/MDS-7301 / https://st.yandex-team.ru/MDS-7302 for detail.
# MUST be deprecated in future!

#ELLIPTICS:STORAGE

/etc/netconfig.d/97-uptun.py:
    file.managed:
        - source: salt://units/tune_tun/files/etc/netconfig.d/97-uptun.py
        - user: root
        - group: root
        - mode: 755

/etc/sysctl.d/ZZ-uptun.conf:
    file.managed:
        - source: salt://units/tune_tun/files/etc/sysctl.d/ZZ-uptun.conf
        - user: root
        - group: root
        - mode: 644

{% if fqdn == 's00vla.storage.yandex.net' %}

/etc/netconfig.d/98-uptun.py:
    file.managed:
        - source: salt://units/tune_tun/files/etc/netconfig.d/98-uptun.py
        - user: root
        - group: root
        - mode: 755

{% endif %}
