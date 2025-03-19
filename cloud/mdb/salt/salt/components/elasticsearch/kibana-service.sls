include:
    - .certs

kibana-service-req:
    test.nop

kibana-restart-service-req:
    test.nop

/etc/kibana/kibana.yml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kibana.yml
        - makedirs: True
        - user: root
        - group: kibana
        - mode: 640
        - require_in:
            - test: kibana-service-req

{% set stop_kibana = salt.pillar.get('stop-kibana-for-upgrade', False) %}
{% if not stop_kibana and salt.pillar.get('data:elasticsearch:kibana:enabled', False) %}

{% if salt.pillar.get('service-restart') and salt.mdb_elasticsearch.version() == '7.11.2' %}
kibana-stop:
    cmd.run:
        - name: service kibana stop
        - require:
            - test: kibana-service-req
            - test: kibana-restart-service-req
        - required_in:
            - service: kibana-service
{% endif %}


kibana-service:
    service.running:
        - name: kibana
        - enable: True
        - require:
            - test: kibana-service-req
            - test: certs-ready
        - watch:
            - file: /etc/kibana/kibana.yml
            - file: /etc/elasticsearch/certs/server.key
{% else %}

kibana-service-dead:
    service.dead:
        - name: kibana
        - enable: False
        - require:
            - test: kibana-service-req

{% endif %}
