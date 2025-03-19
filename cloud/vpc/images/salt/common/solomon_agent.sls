solomon-agent:
  pkg.installed:
    - pkgs:
      - yc-solomon-agent-systemd
  service.running:
    - enable: True

solomon-agent-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/agent.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/solomon-agent/agent.conf
    - makedirs: True
    - require:
      - pkg: solomon-agent

solomon-agent-system-service-config:
  file.managed:
    - name: /etc/yc/solomon-agent-systemd/system.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/solomon-agent/system.conf
    - makedirs: True
    - require:
      - pkg: solomon-agent

solomon-agent-systemd-memory-limit:
  file.managed:
    - name: /etc/systemd/system/solomon-agent.service.d/10-memory-limit.conf
    - source: salt://{{ slspath }}/files/solomon-agent/10-memory-limit.conf
    - makedirs: True
    - require:
      - pkg: solomon-agent
  
