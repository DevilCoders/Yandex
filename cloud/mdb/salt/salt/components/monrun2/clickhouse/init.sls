{% set ch_system_queues_cfg = salt.grains.filter_by({
'Debian': {
    'triggers': {
        'default': {
            'merges_in_queue': {
                'warn': 10,
                'crit': 20
            },
            'future_parts': {
                'warn': 10,
                'crit': 20
            },
            'parts_to_check': {
                'warn': 10,
                'crit': 20
            },
            'queue_size': {
                'warn': 10,
                'crit': 20
            },
            'inserts_in_queue': {
                'warn': 10,
                'crit': 20
            }
        }
    }
  }
}, merge=salt.pillar.get('data:monrun:ch_system_queues')) %}

monrun-clickhouse-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - require:
            - pkg: monrun-clickhouse-packages

include:
    - .config

add-monitor-clickhouse-group:
    user.present:
        - name: monitor
        - groups:
            - clickhouse
        - require:
            - pkg: clickhouse-packages
        - watch_in:
            - service: juggler-client

monrun-clickhouse-packages:
    pkg.installed:
        - pkgs:
            - python3.6
            - python3.6-minimal
            - libpython3.6-stdlib
            - libpython3.6-minimal
            - python3-requests
            - python3-yaml
            - python3-openssl
        - reload_modules: true

/etc/monitoring/ch_system_queues.yaml:
    file.managed:
        - contents: |
            {{ ch_system_queues_cfg | yaml(False) | indent(12) }}
        - mode: 644
        - user: monitor
        - group: monitor
        - makedirs: True

/etc/logrotate.d/clickhouse-monitoring:
    file.managed:
        - source: salt://{{ slspath }}/logrotate.conf
        - mode: 644
        - require:
            - file: monrun-clickhouse-configs

monrun-clickhouse-cleanup:
    file.absent:
        - names:
            - /etc/monrun/conf.d/ch_ping_a.conf
            - /usr/local/yandex/monitoring/ch_ping.py
            - /etc/monrun/conf.d/ch_geobase_a.conf
            - /usr/local/yandex/monitoring/ch_geobase.py
            - /etc/monrun/conf.d/ch_replication_lag_a.conf
            - /usr/local/yandex/monitoring/ch_replication_lag.py
            - /etc/monrun/conf.d/ch_ro_replica_a.conf
            - /usr/local/yandex/monitoring/ch_ro_replica.py
            - /etc/monrun/conf.d/ch_system_queues_a.conf
            - /usr/local/yandex/monitoring/ch_system_queues.py
            - /etc/monrun/conf.d/ch_core_dumps_a.conf
            - /usr/local/yandex/monitoring/ch_core_dumps.py
            - /etc/monrun/conf.d/ch_dist_tables_a.conf
            - /usr/local/yandex/monitoring/ch_dist_tables.py
            - /etc/monrun/conf.d/ch_log_errors_a.conf
            - /usr/local/yandex/monitoring/ch_log_errors.sh
            - /etc/monrun/conf.d/ch_resetup_state_a.conf
            - /usr/local/yandex/monitoring/ch_resetup_state.py
            - /etc/monrun/conf.d/ch_dist_table_size.conf
            - /usr/local/yandex/monitoring/ch_dist_table_size.py
            - /etc/monrun/conf.d/ch_backup_age.conf
            - /usr/local/yandex/monitoring/ch_backup_age.py
            - /usr/local/yandex/monitoring/tls.py
