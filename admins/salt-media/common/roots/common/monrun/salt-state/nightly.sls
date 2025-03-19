include:
  - .check
  - .packages

/etc/cron.d/nightly-salt-state-check:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        SHELL=/bin/bash
        PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
        # Regular nightly highstate check
        #          field          allowed values
        #          -----          --------------
        #----------minute         0–59
        # ---------hour           0–23
        # |  ------day of month   1–31
        # |  | ----month          1–12 (or names)
        # |  | | ,-day of week    0–7 (0 or 7 is Sun)        3600 * 6 = 21600 - random sleep 0s - 6 hours
        0 23 * * * root /usr/local/bin/monrun-salt-state-check.sh -s 21600 &>/dev/null

salt-state:
  monrun.present:
    - command: grep '^[0-9]' /dev/shm/monrun-salt-state.result 2>/dev/null || echo "1;Unknown"
    - execution_interval: {{ pillar.get('salt_state_execution_interval', 810) }}
    - execution_timeout: {{ pillar.get('salt_state_execution_timeout', 400) }}
