include:
    - .certs

dashboards-service-req:
    test.nop

dashboards-restart-service-req:
    test.nop


dashboards-configuration:
    file.recurse:
        - name: /etc/opensearch-dashboards
        - source: salt://{{ slspath }}/conf/etc/opensearch-dashboards
        - template: jinja
        - user: opensearch-dashboards
        - group: opensearch-dashboards
        - dir_mode: 700
        - file_mode: 600
        - recurse:
            - user
            - group
            - mode
        - include_empty: true
        - clean: true
        - require_in:
            - test: dashboards-service-req

{% set stop_dashboards = salt.pillar.get('stop-kibana-for-upgrade', False) %}
{% if not stop_dashboards and salt.pillar.get('data:opensearch:dashboards:enabled', False) %}

# {% if salt.pillar.get('service-restart') and salt.mdb_opensearch.version() == '7.11.2' %}
# dashboards-stop:
#     cmd.run:
#         - name: service opensearch-dashboards stop
#         - require:
#             - test: dashboards-service-req
#             - test: dashboards-restart-service-req
#         - required_in:
#             - service: dashboards-service
# {% endif %}


dashboards-service:
    service.running:
        - name: opensearch-dashboards
        - enable: True
        - require:
            - test: dashboards-service-req
            - test: certs-ready
        - watch:
            - file: dashboards-configuration
            - file: /etc/opensearch/certs/server.key
{% else %}

dashboards-service-dead:
    service.dead:
        - name: opensearch-dashboards
        - enable: False
        - require:
            - test: dashboards-service-req

{% endif %}
