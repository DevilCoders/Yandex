#{% set cgroup = grains['conductor']['group'] %}
#{% set dc = grains['conductor']['root_datacenter'] %}
  
#ELLIPTICS:PROXY & APE:FRONT

/etc/modules.d/03-bbr:
    file.managed:
        - source: salt://units/kernel_tune/files/etc/modules.d/03-bbr
        - user: root
        - group: root
        - mode: 644

/etc/sysctl.d/80-bbr.conf:
    file.managed:
        - source: salt://units/kernel_tune/files/etc/sysctl.d/80-bbr.conf
        - user: root
        - group: root
        - mode: 644

