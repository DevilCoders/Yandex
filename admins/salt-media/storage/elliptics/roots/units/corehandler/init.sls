{% set unit = 'corehandler' %}

{% for file in pillar.get('corehandler-config-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{unit}}{{file}}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

kernel.core_pattern:
  sysctl.present:
    - name: "kernel.core_pattern"
    - value: "|/usr/bin/corehandler %P %s %h %c %t %e"
    - config: "/etc/sysctl.d/ZZ-corehandler.conf"

kernel.core_pipe_limit:
  sysctl.present:
    - name: "kernel.core_pipe_limit"
    - value: 20
    - config: "/etc/sysctl.d/ZZ-corehandler.conf"
