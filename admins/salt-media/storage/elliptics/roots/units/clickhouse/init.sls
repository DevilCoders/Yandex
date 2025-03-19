# FIXME: copy paste from ape

# FIXME: not about clickhouse
# /etc/sysctl.d/60-metadisk-mdadm-resync.conf:
#   file.managed:
#     - source: salt://log-storage/clickhouse/etc/sysctl.d/60-metadisk-mdadm-resync.conf
#
# /etc/sysctl.d/60-ipvs_tun.conf:
#   file.managed:
#     - source: salt://log-storage/clickhouse/etc/sysctl.d/60-ipvs_tun.conf

/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/monrun/conf.d/

/etc/cron.d/clickhouse-clean:
  file.absent

/etc/clickhouse-server/config.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.xml

/etc/clickhouse-server/dictionary.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/dictionary.xml

/etc/clickhouse-server/users.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/users.xml

/etc/clickhouse-server/config.d:
  file.directory:
  - dir_mode: 755

/etc/clickhouse-server/config.d/hostname.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/hostname.xml
    - template: jinja

/etc/clickhouse-server/config.d/graphite.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/graphite.xml

/etc/clickhouse-server/config.d/max_size_to_merge.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/max_size_to_merge.xml

/etc/clickhouse-server/config.d/dictionaries.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/dictionaries.xml

/etc/clickhouse-server/config.d/replica_macros.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/replica_macros.xml
    - template: jinja

# TODO: generate?
/etc/clickhouse-server/config.d/sharding.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/sharding.xml

/etc/clickhouse-server/config.d/zookeeper.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/zookeeper.xml
    - template: jinja

/etc/clickhouse-server/config.d/keeper.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/clickhouse-server/config.d/keeper.xml
    - template: jinja

clickhouse-replica-check:
  file.managed:
    - name: /usr/local/bin/clickhouse-replica-check.sh
    - source: salt://{{ slspath }}/files/usr/local/bin/clickhouse-replica-check.sh
    - mode: 755

  monrun.present:
    - command: "/usr/local/bin/clickhouse-replica-check.sh"
    - execution_interval: 300
    - execution_timeout: 60
    - type: clickhouse

clickhouse-logs-transfer:
  file.managed:
    - name: /usr/local/bin/clickhouse-logs-transfer.sh
    - source: salt://{{ slspath }}/files/usr/local/bin/clickhouse-logs-transfer.sh
    - mode: 755

  monrun.present:
    - command: "/usr/local/bin/clickhouse-logs-transfer.sh"
    - execution_interval: 300
    - execution_timeout: 60
    - type: clickhouse

# remove file from old ape configs
/etc/sysctl.d/60-metadisk-mdadm-resync.conf:
  file:
    - absent

/etc/sysctl.d/60-raid-sync-speed-limit.conf:
  file.managed:
    - name: /etc/sysctl.d/60-raid-sync-speed-limit.conf
    - user: root
    - group: root
    - mode: 644
    - contents: |
        # Raid resync can drop disk performance too hard even on low min values.
        dev.raid.speed_limit_min=1000
        dev.raid.speed_limit_max=20000000

sysctl refresh:
  cmd.run:
    - name: /sbin/sysctl --load
    - onchanges:
      - file: /etc/sysctl.d/60-raid-sync-speed-limit.conf
