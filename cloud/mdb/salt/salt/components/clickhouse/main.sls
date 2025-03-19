{% set array_for_data      = salt.pillar.get('data:array_for_data', '/dev/md1') %}
{% set mountpoint_for_data = salt.pillar.get('data:mountpoint_for_data', '/var/lib/clickhouse') %}
{% set dbaas               = salt.pillar.get('data:dbaas', {}) %}
{% set ch_version          = salt.mdb_clickhouse.version() %}
{% set zk_hosts            = salt.mdb_clickhouse.zookeeper_hosts() %}
{% set zk_root             = salt.mdb_clickhouse.zookeeper_root() %}
{% set schemas             = salt.pillar.get('data:clickhouse:schemas', []) %}
{% set initialize_s3       = salt.pillar.get('data:clickhouse:initialize_s3', True) %}
{% set resetup_possible    = salt.mdb_clickhouse.resetup_possible() %}

{% if salt.pillar.get('restart-check', True) and not salt.pillar.get('service-restart', False) %}
clickhouse-packages-restart-check:
    mdb_clickhouse.check_restart_required:
        - require_in:
            - pkg: clickhouse-packages
{% endif %}

clickhouse-packages:
    pkg.installed:
        - pkgs:
            - clickhouse-server: {{ ch_version }}
            - clickhouse-client: {{ ch_version }}
            - clickhouse-common-static: {{ ch_version }}
{% if salt.dbaas.is_porto() or salt.mdb_clickhouse.do_install_debug_symbols() %}
            - clickhouse-common-static-dbg: {{ ch_version }}
{% endif %}
            - mdb-ch-tools: '1.9715363'
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
            - python-boto3
            - python3-boto3
            - python3-tenacity
            - clickhouse-geodb: 2019.12.18-160618
            - catboost-model-lib: 0.17.3
{% if salt.dbaas.is_porto() %}
            - yandex-clickhouse-dictionary-yt: '8389625'
{% endif %}
            - unixodbc
            - odbcinst
            - odbc-postgresql
            - libpq5: '10.7-100-yandex.44181.a093e14'
            - kafkacat
            - ncdu
            - jq

clickhouse-post-install-cleanup:
    file.absent:
        - names:
            - /usr/lib/debug/usr/bin/clickhouse-odbc-bridge
            - /etc/init.d/clickhouse-server
            - /etc/clickhouse-server/config.d/data-paths.xml
            - /etc/clickhouse-server/config.d/logger.xml
            - /etc/clickhouse-server/config.d/openssl.xml
            - /etc/clickhouse-server/config.d/user-directories.xml
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server

/etc/systemd/system/clickhouse-server.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server
        - source: salt://{{ slspath }}/conf/clickhouse-server.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/clickhouse-server.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server
        - source: salt://{{ slspath }}/conf/clickhouse-server.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/clickhouse-pre_start.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server
        - source: salt://{{ slspath }}/conf/clickhouse_pre_start.sh
        - mode: 755

/usr/local/yandex/clickhouse-stop-wait.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - file: /etc/systemd/system/clickhouse-server.service
        - source: salt://{{ slspath }}/conf/clickhouse-stop-wait.sh
        - mode: 755

/usr/local/yandex/clickhouse-wd.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - file: /etc/cron.d/clickhouse-server
        - source: salt://{{ slspath }}/conf/clickhouse-wd.sh
        - mode: 755

{% if salt.mdb_clickhouse.has_separated_keeper() %}
/etc/systemd/system/clickhouse-keeper.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-keeper
        - source: salt://{{ slspath }}/conf/clickhouse-keeper.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/clickhouse-keeper.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-keeper
        - source: salt://{{ slspath }}/conf/clickhouse-keeper.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/clickhouse-keeper-pre-start.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-keeper
        - source: salt://{{ slspath }}/conf/clickhouse-keeper-pre-start.sh
        - mode: 755

/usr/local/yandex/clickhouse-keeper-stop-wait.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - file: /etc/systemd/system/clickhouse-keeper.service
        - source: salt://{{ slspath }}/conf/clickhouse-keeper-stop-wait.sh
        - mode: 755

/usr/local/yandex/clickhouse-keeper-wd.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - file: /etc/cron.d/clickhouse-keeper
        - source: salt://{{ slspath }}/conf/clickhouse-keeper-wd.sh
        - mode: 755
{% else %}
clickhouse-keeper-cleanup:
    file.absent:
        - names:
            - /etc/systemd/system/clickhouse-keeper.service
            - /lib/systemd/system/clickhouse-keeper.service
            - /usr/local/yandex/clickhouse-keeper-pre-start.sh
            - /usr/local/yandex/clickhouse-keeper-stop-wait.sh
            - /usr/local/yandex/clickhouse-keeper-wd.sh
{% endif %}

{{ mountpoint_for_data }}:
    mount.mounted:
        - name: {{ mountpoint_for_data }}
        - device: {{ array_for_data }}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
        - onlyif:
            - fgrep {{ array_for_data }} /etc/mdadm/mdadm.conf
        - require_in:
            - pkg: clickhouse-packages
    file.directory:
        - user: clickhouse
        - group: clickhouse
        - dir_mode: 775
        - require:
            - mount: {{ mountpoint_for_data }}
            - pkg: clickhouse-packages

{{ mountpoint_for_data }}/lost+found:
    file.absent:
        - require:
            - mount: {{ mountpoint_for_data }}

/etc/clickhouse-server:
    file.directory:
        - user: root
        - group: clickhouse
        - makedirs: True
        - mode: 755
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server

/var/log/clickhouse-server:
    file.directory:
        - user: clickhouse
        - group: clickhouse
        - makedirs: True
        - mode: 775
        - file_mode: 644
        - require:
              - pkg: clickhouse-packages
        - require_in:
              - service: clickhouse-server
        - recurse:
            - user
            - group
            - mode

{% if salt.mdb_clickhouse.has_separated_keeper() %}
/var/log/clickhouse-keeper:
    file.directory:
        - user: clickhouse
        - group: clickhouse
        - makedirs: True
        - mode: 775
        - file_mode: 644
        - require:
              - pkg: clickhouse-packages
        - require_in:
              - service: clickhouse-keeper
        - recurse:
            - user
            - group
            - mode
{% else %}
/var/log/clickhouse-keeper:
    file.absent
{% endif %}

/var/log/clickhouse-monitoring:
    file.directory:
        - user: monitor
        - group: monitor
        - makedirs: True
        - mode: 775
        - file_mode: 644
        - require:
              - pkg: clickhouse-packages
        - require_in:
              - service: clickhouse-server
        - recurse:
            - user
            - group
            - mode

/etc/sudoers.d/monitoring:
    file.managed:
        - source: salt://{{ slspath }}/conf/monitoring.sudoers
        - mode: 440
        - template: jinja
        - require:
              - pkg: clickhouse-packages
        - require_in:
              - service: clickhouse-server

include:
    - .configs.s3-credentials
    - .configs.server-config
    - .configs.server-cluster
    - .configs.server-users
    - .configs.dictionaries
    - .configs.storage
    - .service
    - .databases-sync
    - .models
    - .format-schemas
    - .geobase
    - .sql-users
{% if initialize_s3 and resetup_possible %}
    - .resetup
{% endif %}

extend:
    clickhouse-server-config-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages
                - file: {{ mountpoint_for_data }} {# Needed so that CH has permissions to write down preprocessed config #}

    clickhouse-server-cluster-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-server-users-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-dictionaries-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-models-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-format-schemas-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-geobase-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-storage-req:
        test.nop:
            - require:
                - pkg: clickhouse-packages

    clickhouse-server-config-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-server-cluster-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-server-users-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-dictionaries-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-models-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-geobase-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-storage-ready:
        test.nop:
            - require_in:
                - service: clickhouse-server

    clickhouse-sync-databases-req:
        test.nop:
            - require:
                - cmd: clickhouse-server

    clickhouse-sql-users-req:
        test.nop:
            - require:
                - test: clickhouse-sync-databases-ready

/etc/cron.d/clickhouse-server:
    file.managed:
        - source: salt://{{ slspath }}/conf/clickhouse-server.cron
        - template: jinja
        - mode: 644
        - require:
            - cmd: clickhouse-server

/usr/local/yandex/ch_wait_started.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ch_wait_started.py
        - mode: 755
        - makedirs: True
        - require:
            - file: {{ mountpoint_for_data }}
        - require_in:
            - cmd: clickhouse-server

{% if salt.mdb_clickhouse.has_separated_keeper() %}
/etc/cron.d/clickhouse-keeper:
    file.managed:
        - source: salt://{{ slspath }}/conf/clickhouse-keeper.cron
        - template: jinja
        - mode: 644
        - require:
            - cmd: clickhouse-server

/usr/local/yandex/ch_keeper_wait_started.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ch_keeper_wait_started.py
        - mode: 755
        - makedirs: True
        - require:
            - file: {{ mountpoint_for_data }}
        - require_in:
            - cmd: clickhouse-server
{% else %}
/etc/cron.d/clickhouse-keeper:
    file.absent

/usr/local/yandex/ch_keeper_wait_started.py:
    file.absent
{% endif %}

/usr/local/yandex/ch_wait_replication_sync.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ch_wait_replication_sync.py
        - mode: 755
        - makedirs: True
        - require_in:
            - cmd: clickhouse-server

/etc/clickhouse-client/config.xml:
    file.managed:
        - template: jinja
        - source: salt://components/clickhouse/etc/clickhouse-client/config.xml
        - user: clickhouse
        - mode: 644
        - makedirs: True
        - require:
            - pkg: clickhouse-packages
{% if salt.pillar.get('service-restart', False) %}
            - cmd: clickhouse-stop
{% endif %}
        - require_in:
            - service: clickhouse-server

{% if zk_hosts and zk_root and not salt.mdb_clickhouse.has_embedded_keeper() %}
clickhouse-zk-up:
    mdb_clickhouse.wait_for_zookeeper:
        - zk_hosts: {{ zk_hosts }}
        - wait_timeout: {{ salt.pillar.get('data:clickhouse:zk_wait_timeout', 600) }}
        - require:
            - pkg: clickhouse-packages

clickhouse-zk-root:
    zookeeper.present:
        - name: {{ zk_root }}
        - value: ''
        - makepath: True
        - hosts: {{ zk_hosts }}
        - require:
            - mdb_clickhouse: clickhouse-zk-up
        - require_in:
            - service: clickhouse-server
{% endif %}

{# TODO: clean up enable_cap_net_admin flag when cap_net_admin is revoked from all clusters #}
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' and not salt.pillar.get('data:clickhouse:enable_cap_net_admin') %}
clickhouse-capabilities:
    cmd.run:
        - name: setcap 'cap_ipc_lock,cap_sys_nice+ep' /usr/bin/clickhouse
        - unless: getcap /usr/bin/clickhouse | grep -q 'cap_ipc_lock,cap_sys_nice+ep'
        - require:
            - pkg: clickhouse-packages
        - require_in:
            - service: clickhouse-server
{% endif %}

{% for database in schemas %}
/usr/local/yandex/clickhouse/{{ database }}.sql:
    file.managed:
        - source: salt://components/clickhouse/databases/{{ database }}.sql
        - makedirs: True
        - require:
            - cmd: clickhouse-server

apply_{{ database }}:
    cmd.wait:
        - name: 'cat /usr/local/yandex/clickhouse/{{ database }}.sql | clickhouse-client -A -m -n'
        - watch:
            - file: /usr/local/yandex/clickhouse/{{ database }}.sql
{% endfor %}

/etc/logrotate.d/clickhouse-server:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - template: jinja
        - mode: 644
        - require:
            - pkg: clickhouse-packages
            - file: /usr/local/yandex/log_curator.py

/etc/cron.d/log-curator:
    file.managed:
        - source: salt://{{ slspath }}/conf/log-curator.cron
        - template: jinja
        - mode: 644
        - require:
            - file: /usr/local/yandex/log_curator.py

{% if salt.grains.get('virtual') == 'lxc'%}
/etc/cron.d/clickhouse_porto_chown:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             @reboot root chown clickhouse.clickhouse /var/lib/clickhouse >/dev/null 2>&1
        - mode: 644
{% endif %}

{% if initialize_s3 %}
initialized_flag:
    mdb_s3.object_present:
        - name: {{ salt.mdb_clickhouse.initialized_flag_name() }}
        - require:
            - test: clickhouse-sync-databases-ready
{% endif %}

ensure_mdb_tables:
    mdb_clickhouse.ensure_mdb_tables:
        - require:
            - cmd: clickhouse-server
            - test: clickhouse-sync-databases-ready

clickhouse-component-cleanup:
    file.absent:
        - names:
            - /etc/yandex/ch-resetup/ch-resetup.py
            - /etc/yandex/ch-resetup/ch-resetup.sh
            - /usr/local/yandex/ch-resetup/ch-resetup.py
            - /usr/local/yandex/clickhouse-autorecovery.py
            - /etc/cron.d/clickhouse-autorecovery
            - /etc/logrotate.d/clickhouse-autorecovery
            - /etc/cron.yandex/ch-wait-zk.sh

{% if salt.mdb_clickhouse.has_embedded_keeper() %}
/root/.zkpass:
    file.managed:
    - source: salt://components/zk/conf/zkpass
    - template: jinja
    - mode: 600
    - user: root
    - group: root
{% endif %}
