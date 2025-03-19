elasticsearch-service-req:
    test.nop

elasticsearch-service-ready:
    test.nop

elasticsearch-service:
    service.running:
        - name: elasticsearch
        - enable: True
        - require:
            - test: elasticsearch-service-req
    module.run:
        - name: mdb_elasticsearch.wait_for_status
        - status: {{ salt.pillar.get('wait_for_status', 'yellow') }}
        - onchanges:
            - service: elasticsearch-service
        - required_in:
            - test: elasticsearch-service-ready

elasticsearch-license:
    mdb_elasticsearch.ensure_license:
        - edition: {{ salt.mdb_elasticsearch.edition() }}
        - require:
            - module: elasticsearch-service

{% if salt.pillar.get('service-restart') %}
include:
    - .elasticsearch-restart
{% endif %}
