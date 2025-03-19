include:
    - components.opensearch.plugins
    - components.opensearch.config
    - components.opensearch.keystore
    - components.opensearch.service
    - components.opensearch.nginx
    - components.opensearch.dashboards-service
    - components.dbaas-billing.billing
#    - components.opensearch.extensions


extend:
    opensearch-configuration:
        file.recurse:
            - require:
                - mdb_opensearch: opensearch-plugins

    reload-keystore:
        module.run:
            - require:
                - module: opensearch-service

    opensearch-service-req:
        test.nop:
            - require:
                - test: opensearch-keystore-ready
                - test: certs-ready
                - file: opensearch-configuration
                - mdb_opensearch: opensearch-plugins
                
# extend:
#     opensearch-service:
#         service.running:
#             - watch:
#                 - file: /etc/opensearch/opensearch.yml
#                 - file: /etc/opensearch/jvm.options
#                 - file: /etc/opensearch/log4j2.properties
#                 - file: /etc/default/opensearch
