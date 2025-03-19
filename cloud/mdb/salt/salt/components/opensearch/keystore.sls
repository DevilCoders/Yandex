opensearch-keystore-req:
    test.nop

opensearch-keystore-ready:
    test.nop

create-keystore:
    cmd.run:
      - name: /usr/share/opensearch/bin/opensearch-keystore create
      - cwd: /etc/opensearch
      - creates: /etc/opensearch/opensearch.keystore
      - require_in: 
          - test: opensearch-keystore-ready
      - require:
          - test: opensearch-keystore-req

ensure-keystore:
    mdb_opensearch.ensure_keystore:
      - settings: {{ salt.mdb_opensearch.keystore_settings() }}
      - require:
          - cmd: create-keystore
      - require_in: 
          - test: opensearch-keystore-ready

/etc/opensearch/opensearch.keystore:
    file.managed:
      - replace: False
      - mode: 440
      - user: opensearch
      - group: opensearch
      - require:
          - cmd: create-keystore
          - mdb_opensearch: ensure-keystore
      - require_in: 
          - test: opensearch-keystore-ready

reload-keystore:
    module.run:
      - name: mdb_opensearch.reload_secure_settings
      - require:
          - file: /etc/opensearch/opensearch.keystore
      - onchanges:
          - mdb_opensearch: ensure-keystore
