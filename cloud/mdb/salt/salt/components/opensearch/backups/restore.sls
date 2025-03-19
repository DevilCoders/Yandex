elasticsearch-restore-req:
    test.nop

restore-repository:
    mdb_opensearch.ensure_repository:
        - reponame: yc-automatic-restore
        - settings: 
            client: yc-automatic-backups
            base_path: backups
            bucket: {{ salt.mdb_opensearch.pillar('restore_from:bucket') }}
            readonly: true
        - require:
            - test: elasticsearch-restore-req

do-restore:
    module.run:
        - name: mdb_opensearch.do_restore
        - backup_id: {{ salt.mdb_opensearch.pillar('restore_from:backup') }}
    require:
        - mdb_opensearch: restore-repository
