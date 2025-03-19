rsync-status:
  monrun.present:
    - command: /usr/bin/daemon_check.sh rsync
    - execution_interval: 300
