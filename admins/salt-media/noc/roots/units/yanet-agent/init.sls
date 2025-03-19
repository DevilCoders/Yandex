{% from "units/percent.sls" import packages_map %}

/etc/yanet-agent/.env:
  file.managed:
    - template: jinja
    - makedirs: True
    - mode: 600
    - contents: |
        TVM_ID=2032161
        TVM_SECRET={{pillar['sec']['client_secret']}}

yanet-agent.service:
  file.managed:
    - name: /lib/systemd/system/yanet-agent.service

yanet-agent:
  pkg.installed:
    - version: {{packages_map["yanet-agent"].version}}
    - allow_updates: True
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: yanet-agent.service
  service.running:
    - enable: True
    - restart: True
    - wait3: True
    - init_delay: 3
    - watch:
      - pkg: yanet-agent
# Задел на будущее, move yanet-agent configs to salt from ann please
#      - file: /etc/yanet-agent/config.yaml
#      - file: /etc/yanet-agent/firewall.txt.jinja
