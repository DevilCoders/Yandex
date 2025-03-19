{% from "components/postgres/pg.jinja" import pg with context %}

umount_array_for_xlogs:
    cmd.run:
        - name: umount {{ pg.wal_dir_path }}
        - require_in:
            - postgresql_cmd: pg-init
        - onlyif:
            - fgrep {{ pg.wal_dir_path }} /proc/mounts
        - unless:
            - ls {{ pg.data }}/postgresql.conf
        - onchages:
            - file: umount_array_for_xlogs
    file.absent:
        - name: {{ pg.wal_dir_path }}
        - require_in:
            - postgresql_cmd: pg-init
        - unless:
            - ls {{ pg.data }}/postgresql.conf

remount_array_for_xlogs:
    cmd.run:
        - name: 'mkdir -p /tmp/backupxlogs; mv {{ pg.wal_dir_path }}/* /tmp/backupxlogs/'
        - runas: postgres
        - group: postgres
        - require:
            - cmd: umount_array_for_xlogs
            - postgresql_cmd: pg-init
        - unless:
            - fgrep {{ pg.wal_dir_path }} /proc/mounts
        - onlyif:
            - ls {{ pg.wal_dir_path }}/*
    mount.mounted:
        - name: {{ pg.wal_dir_path }}
        - device: {{ salt['pillar.get']('data:array_for_xlogs', '/dev/md3')}}
        - fstype: ext4
        - opts: noatime,nodiratime
        - persist: True
        - pass_num: 1
        - require:
            - cmd: remount_array_for_xlogs
        - require_in:
            - cmd: postgresql-service
        - unless:
            - fgrep {{ pg.wal_dir_path }} /proc/mounts

restore_xlogs:
    file.directory:
        - name: {{ pg.wal_dir_path }}
        - user: postgres
        - group: postgres
        - mode: 0700
        - require_in:
            - cmd: restore_xlogs
        - require:
            - mount: remount_array_for_xlogs
    cmd.run:
        - name: mv /tmp/backupxlogs/* {{ pg.wal_dir_path }}/
        - runas: postgres
        - group: postgres
        - require:
            - mount: remount_array_for_xlogs
        - require_in:
            - service: postgresql-service
        - onlyif:
            - ls /tmp/backupxlogs/*
