td-agent:
  user.present:
    - shell: /bin/false
    - home: /var/lib/td-agent
    - gid_from_name: True
    - groups:
      - systemd-journal

/etc/td-agent/config.d:
  file.directory:
    - makedirs: True
    - user: root
    - group: root

/var/log/fluent:
  file.directory:
    - user: td-agent
    - group: td-agent

/var/log/pos:
  file.directory:
    - user: td-agent
    - group: td-agent

/etc/logrotate.d/fluentd:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - source: salt://configs/td-agent/fluentd.logrotate

/etc/td-agent/td-agent.conf:
  file.managed:
    - source: salt://configs/td-agent/td-agent.conf

/etc/td-agent/config.d/containers.input.conf:
  file.managed:
    - source: salt://configs/td-agent/config.d/containers.input.conf

/etc/td-agent/config.d/monitoring.conf:
  file.managed:
    - source: salt://configs/td-agent/config.d/monitoring.conf

/etc/td-agent/config.d/output.conf:
  file.managed:
    - source: salt://configs/td-agent/config.d/output.conf

/etc/td-agent/config.d/system.input.conf:
  file.managed:
    - source: salt://configs/td-agent/config.d/system.input.conf

/lib/systemd/system/td-agent.service:
  file.managed:
    - source: salt://services/td-agent.service

'td-agent-gem install fluent-plugin-concat -v 2.4.0':
  cmd.run

'td-agent-gem install fluent-plugin-detect-exceptions -v 0.0.12':
  cmd.run

'td-agent-gem install fluent-plugin-systemd -v 1.0.2':
  cmd.run

td-agent-service:
  service.running:
    - name: td-agent.service
    - enable: True
