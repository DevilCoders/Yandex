certs-ready:
    test.nop

es-certs:
    group.present:
        - require_in:
            - test: certs-ready

# for elasticsearch certs must be placed in config dir
/etc/elasticsearch/certs:
    file.directory:
        - user: root
        - group: es-certs
        - makedirs: True
        - mode: 755
        - require:
            - group: es-certs
        - require_in:
            - test: certs-ready

/etc/elasticsearch/certs/{{salt.mdb_elasticsearch.fqdn()}}.key:
    file.managed:
        - contents_pillar: cert.key
        - user: root
        - group: es-certs
        - mode: 640
        - require:
            - file: /etc/elasticsearch/certs
        - require_in:
            - test: certs-ready

/etc/elasticsearch/certs/{{salt.mdb_elasticsearch.fqdn()}}.crt:
    file.managed:
        - contents_pillar: cert.crt
        - user: root
        - group: es-certs
        - mode: 644
        - require:
            - file: /etc/elasticsearch/certs
        - require_in:
            - test: certs-ready

/etc/elasticsearch/certs/server.key:
    file.managed:
        - contents_pillar: cert.key
        - user: root
        - group: es-certs
        - mode: 640
        - require:
            - file: /etc/elasticsearch/certs
        - require_in:
            - test: certs-ready

/etc/elasticsearch/certs/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - user: root
        - group: es-certs
        - mode: 644
        - require:
            - file: /etc/elasticsearch/certs
        - require_in:
            - test: certs-ready

/etc/elasticsearch/certs/ca.pem:
    file.managed:
        - contents_pillar: cert.ca
        - user: root
        - group: es-certs
        - mode: 644
        - require:
            - file: /etc/elasticsearch/certs
        - require_in:
            - test: certs-ready
