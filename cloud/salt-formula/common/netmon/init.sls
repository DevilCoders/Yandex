netmon_package:
  yc_pkg.installed:
    - pkgs:
      - yandex-netmon-agent

{# NOTE(k-zaitsev): Run this command once, when package is reinstalled
   https://docs.saltstack.com/en/latest/ref/states/all/salt.states.cmd.html#should-i-use-cmd-run-or-cmd-wait #}
netmon_fixcap:
  cmd.run:
    - name: "/usr/local/bin/fixcap --path /usr/local/bin/netmon-agent"
    - onchanges:
      - yc_pkg: netmon_package

/etc/systemd/system/netmon-agent.service:
  file.managed:
    - source: salt://{{ slspath }}/conf/netmon-agent.service
    - require:
      - yc_pkg: netmon_package

/etc/netmon-agent-config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/conf/netmon-agent-config.yaml

/var/lib/netmon:
  file.directory:
    - user: nobody
    - group: nogroup
    - dir_mode: 755

netmon-agent:
  service.running:
    - enable: True
    - require:
      - file: /etc/systemd/system/netmon-agent.service
      - file: /var/lib/netmon
      - file: /etc/netmon-agent-config.yaml
      - cmd: netmon_fixcap
    - watch:
      - file: /etc/systemd/system/netmon-agent.service
      - file: /etc/netmon-agent-config.yaml

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
