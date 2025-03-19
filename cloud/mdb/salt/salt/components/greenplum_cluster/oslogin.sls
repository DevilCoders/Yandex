l2_sudoers:
  file.accumulated:
    - filename: /etc/sudoers.d/l2_support
    - text:
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/less /var/log/odyssey/odyssey.log*'
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/zless /var/log/odyssey/odyssey.log*'
        - 'ALL = NOPASSWD: /bin/cat /var/log/odyssey/odyssey.log*'
        - 'ALL = NOPASSWD: /bin/zcat /var/log/odyssey/odyssey.log*'
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/less /var/lib/greenplum/data1/master/gpseg-1/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/zless /var/lib/greenplum/data1/master/gpseg-1/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: /bin/cat /var/lib/greenplum/data1/master/gpseg-1/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: /bin/zcat /var/lib/greenplum/data1/master/gpseg-1/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/less /var/lib/greenplum/data1/mirror/gpseg[0-9]/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: NOEXEC: /usr/bin/zless /var/lib/greenplum/data1/mirror/gpseg[0-9]/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: /bin/cat /var/lib/greenplum/data1/mirror/gpseg[0-9]/pg_log/greenplum-6-data.csv*'
        - 'ALL = NOPASSWD: /bin/zcat /var/lib/greenplum/data1/mirror/gpseg[0-9]/pg_log/greenplum-6-data.csv*'
    - require_in:
      - file: /etc/sudoers.d/l2_support

gp_dba_sudoers:
  file.accumulated:
    - filename: /etc/sudoers.d/l2_support
    - text:
        - 'ALL = NOPASSWD: ALL'
    - require_in:
      - file: /etc/sudoers.d/l2_support
