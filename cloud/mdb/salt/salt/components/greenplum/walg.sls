{% from "components/greenplum/map.jinja" import gpdbvars with context %}

include:
    - .walg-config
{% if salt['pillar.get']('do-backup') %}
    - .run-backup
{% endif %}
{% if salt['pillar.get']('data:walg_periodic_backups', True) %}
    - .walg-cron
{% else %}

/etc/cron.d/gp_walg_backup_push:
    file.absent
/etc/cron.d/gp_walg_create_restore_point:
    file.absent
{% endif %}

extend:
{% if salt['pillar.get']('do-backup') %}
    do_s3_backup:
        cmd.run:
            - require:
                - test: greenplum-ready
                - file: /usr/local/yandex/gp_walg_backup_push.py
                - file: /etc/wal-g-backup-push.conf
{% endif %}

    greenplum-walg-config-req:
        test.nop:
            - require:
                - pkg: walg-packages
                - file: /etc/wal-g
    greenplum-walg-config-ready:
        test.nop:
            - require_in:
                - service: greenplum-service

/etc/wal-g/envdir:
    file.absent

{% set environment = salt['pillar.get']('yandex:environment', 'dev') %}
{% if salt['pillar.get']('data:s3:gpg_key')  %}
/etc/wal-g/PGP_KEY:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:s3:gpg_key') | indent(12)}}
        - require:
            - pkg: walg-packages
            - file: /etc/wal-g/wal-g.yaml
        - require_in:
            - service: greenplum-service
{% endif %}

walg-packages:
    pkg.installed:
        - pkgs:
{% set walg_version = salt['pillar.get']('data:walg:version') %}
{% if not walg_version %}
{%     if environment == 'dev' %}
{%     set walg_version = '1292-f504f152' %}
{%     elif environment == 'qa' %}
{%     set walg_version = '1292-f504f152' %}
{%     elif environment == 'prod' %}
{%     set walg_version = '1290-ee0b2e69' %}
{%     else %}
{%     set walg_version = '1290-ee0b2e69' %}
{%     endif %}
{% endif %}
            - wal-g-gp: {{ walg_version }}
            - daemontools
            - python3-dateutil
        - prereq_in:
            - cmd: repositories-ready
        - require_in:
            - service: greenplum-service
            - file: /etc/wal-g/wal-g.yaml

/usr/bin/wal-g:
    file.symlink:
        - target: /usr/bin/wal-g-gp
        - require:
            - pkg: walg-packages

/etc/wal-g:
    file.directory:
        - user: root
        - group: s3users
        - mode: 0750
        - makedirs: True
        - require:
            - group: s3users
        - require_in:
            - file: /etc/wal-g/wal-g.yaml

/usr/local/yandex/gp_walg_backup_push.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_walg_backup_push.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/gp_walg_create_restore_point.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_walg_create_restore_point.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

s3users:
    group.present:
        - members:
            - {{ gpdbvars.gpadmin }}
            - monitor
            - telegraf
        - system: True
        - require:
            - file: /var/lib/greenplum
            - user: telegraf
        - require_in:
            - service: greenplum-service
            - file: /etc/wal-g/wal-g.yaml
        - watch_in:
            - service: telegraf.service

{% if salt['pillar.get']('restore-from:cid') %}
/etc/wal-g/wal-g-restore.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g-restore.yaml
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - group: s3users
            - file: /etc/wal-g
        - require_in:
            - pkg: walg-packages

/etc/wal-g/PGP_KEY_RESTORE:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:restore-from-pillar-data:s3:gpg_key') | indent(12)}}
        - require:
            - group: s3users
            - file: /etc/wal-g
        - require_in:
            - pkg: walg-packages

{%   if salt.pillar.get('gpdb_master') %}
/etc/wal-g/wal-g-restore-config.json:
    fs.file_present:
        - user: root
        - group: s3users
        - mode: 640
        - contents_function: mdb_greenplum.render_walg_restore_config
        - require:
            - group: s3users
            - file: /etc/wal-g
            - pkg: walg-packages

clear-cluster-data-dirs:
    cmd.run:
        - name: >
            gpssh -f /home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/cluster -v -e <<EOF
            rm -rf {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1
            rm -rf {{ gpdbvars.data_folders|first }}/primary/{{ gpdbvars.segprefix }}*
            rm -rf {{ gpdbvars.data_folders|first }}/mirror/{{ gpdbvars.segprefix }}*
            EOF
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - unless:
            - ls /tmp/recovery-state/backup-fetched

wait-for-walg-installed:
    cmd.run:
        - name: gpssh -f /home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/cluster -v -e 'wal-g --version'
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - retry:
            attempts: 10
            until: True
            interval: 60
        - unless:
            - ls /tmp/recovery-state/backup-fetched

backup-fetched:
    cmd.run:
        - name: >
            wal-g backup-fetch --target-user-data='{"backup_id": "{{ salt['pillar.get']('restore-from:backup-id') }}" }' --config /etc/wal-g/wal-g-restore.yaml --restore-config /etc/wal-g/wal-g-restore-config.json  &&
            touch /tmp/recovery-state/backup-fetched
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - require:
            - file: /tmp/recovery-state
            - file: /etc/wal-g/wal-g-restore.yaml
            - file: /etc/wal-g/PGP_KEY_RESTORE
            - fs: /etc/wal-g/wal-g-restore-config.json
            - sls: components.greenplum.install_greenplum
            - sls: components.greenplum.configure_greenplum
            - cmd: gp-cluster-up
            - file: /usr/bin/wal-g
            - cmd: wait-for-walg-installed
            - cmd: clear-cluster-data-dirs
        - require_in:
            - file: {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_hba.conf
        - unless:
            - ls /tmp/recovery-state/backup-fetched

remove-restore-configs-when-greenplum-restored:
    cmd.run:
        - name: >
            rm -f /etc/wal-g/wal-g-restore.yaml &&
            rm -f /etc/wal-g/PGP_KEY_RESTORE &&
            rm -f /etc/wal-g/wal-g-restore-config.json
        - require:
            - cmd: greenplum-restore
{%   endif %}
{% else %}
/etc/wal-g/wal-g-restore.yaml:
    file.absent

/etc/wal-g/PGP_KEY_RESTORE:
    file.absent
{% endif %}
