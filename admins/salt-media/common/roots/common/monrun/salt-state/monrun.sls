salt-state:
  monrun.present:
    - command: /usr/local/bin/monrun-salt-state-check.sh {{ pillar.get('salt_state_command_args', '') }}
    - execution_interval: {{ pillar.get('salt_state_execution_interval', 810) }}
    - execution_timeout: {{ pillar.get('salt_state_execution_timeout', 400) }}
