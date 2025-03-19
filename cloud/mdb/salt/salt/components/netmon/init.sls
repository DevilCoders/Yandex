include:
    - components.monrun2.netmon

netmon-pkg:
    pkg.installed:
        - pkgs:
              - yandex-netmon-agent: '0.9.2.31-8229402'
        - require:
              - cmd: repositories-ready

/var/lib/netmon:
    file.directory:
        - user: nobody
        - group: nogroup
        - mode: '0755'

/etc/netmon-agent-config.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/netmon-agent-config.yaml
        - mode: '0640'
        - user: nobody

/lib/systemd/system/netmon-agent.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/netmon-agent.service
        - mode: '0644'
        - require:
              - pkg: netmon-pkg
              - file: /var/lib/netmon
              - file: /etc/netmon-agent-config.yaml
        - onchanges_in:
              - module: systemd-reload

netmon-agent-service:
    service.running:
        - name: netmon-agent
        - enable: True
        - watch:
              - pkg: netmon-pkg
              - file: /lib/systemd/system/netmon-agent.service
              - file: /etc/netmon-agent-config.yaml
