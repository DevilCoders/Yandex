opensearch-service-req:
    test.nop

opensearch-service-ready:
    test.nop

opensearch-service:
    service.running:
        - name: opensearch
        - enable: True
        - require:
            - test: opensearch-service-req
            - file: opensearch-configuration
    module.run:
        - name: mdb_opensearch.wait_for_status
        - status: {{ salt.pillar.get('wait_for_status', 'yellow') }}
        - onchanges:
            - service: opensearch-service
        - required_in:
            - module: reload-keystore
            - test: opensearch-service-ready

{% if salt.pillar.get('service-restart') %}

###############################################
### gracefully stop service if restart required

mdb_opensearch.flush:
    module.run:
        - name: mdb_opensearch.flush
        - onlyif: # in the case of disk resize service may be stopped
            - service opensearch status
        - require:
            - test: opensearch-service-req

opensearch-stop:
    cmd.run:
        - name: service opensearch stop
        - require:
            - test: opensearch-service-req
            - module: mdb_opensearch.flush
        - require_in:
            - service: opensearch-service

{% endif %}
