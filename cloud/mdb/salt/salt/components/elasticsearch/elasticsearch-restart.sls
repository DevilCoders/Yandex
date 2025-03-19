mdb_elasticsearch.flush:
    module.run:
        - name: mdb_elasticsearch.flush
        - onlyif: # in the case of disk resize service may be stopped
            - service elasticsearch status
        - require:
            - test: elasticsearch-conf-ready

elasticsearch-stop:
    cmd.run:
        - name: service elasticsearch stop
        - require:
            - test: elasticsearch-conf-ready
            - module: mdb_elasticsearch.flush
        - require_in:
            - service: elasticsearch-service
