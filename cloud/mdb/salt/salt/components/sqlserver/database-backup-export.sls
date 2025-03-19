external-storage-config:
    file.managed:
        - name: 'C:\ProgramData\wal-g\external-wal-g.yaml'
        - template: jinja
        - source: salt://{{ slspath }}/conf/external-wal-g.yaml
        - context:
            operation: 'backup-export'
        
backup-exported:
    mdb_sqlserver.backup_exported:
        - walg_config: 'C:\ProgramData\wal-g\wal-g.yaml'
        - walg_ext_config: 'C:\ProgramData\wal-g\external-wal-g.yaml'
        - backup_id: {{ salt['pillar.get']('backup-export:backup_id', 'LATEST')|yaml_encode }}
        - databases: 
            {{ salt['pillar.get']('target-database')|yaml_encode }}: {{ salt['pillar.get']('backup-export:s3:prefix')|yaml_encode }}
        - require:
            - file: external-storage-config

