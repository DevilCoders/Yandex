include:
    - components.elasticsearch.elasticsearch-config
    - components.elasticsearch.elasticsearch-plugins
    - components.elasticsearch.elasticsearch-keystore
    - components.elasticsearch.elasticsearch-service
    - components.elasticsearch.nginx
    - components.elasticsearch.users
    - components.elasticsearch.kibana-service
    - components.dbaas-billing.billing
    - components.elasticsearch.extensions

extend:
    elasticsearch-service:
        service.running:
            - watch:
                - file: /etc/elasticsearch/elasticsearch.yml
                - file: /etc/elasticsearch/jvm.options
                - file: /etc/elasticsearch/log4j2.properties
                - file: /etc/default/elasticsearch
