{% from "components/sqlserver/map.jinja" import sqlserver with context %}

external-storage-config:
    file.managed:
        - name: 'C:\ProgramData\wal-g\external-wal-g.yaml'
        - template: jinja
        - source: salt://{{ slspath }}/conf/external-wal-g.yaml
        - context:
            operation: 'backup-import'
        
backup-imported:
    mdb_sqlserver.backup_imported:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - walg_ext_config: 'C:\ProgramData\wal-g\external-wal-g.yaml'
        - databases: 
            {{ salt['pillar.get']('target-database')|yaml_encode }}: {{ salt['pillar.get']('backup-import:s3:files')|tojson }}
        - require:
            - file: external-storage-config
