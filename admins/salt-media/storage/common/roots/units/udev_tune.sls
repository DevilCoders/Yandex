#{% set cgroup = grains['conductor']['group'] %}
#{% set dc = grains['conductor']['root_datacenter'] %}
  
#ELLIPTICS:PROXY & APE:FRONT

/lib/udev/rules.d/85-iosched.rules:
    file.managed:
        - source: salt://units/udev_tune/files/85-iosched.rules
        - user: root
        - group: root
        - mode: 644

