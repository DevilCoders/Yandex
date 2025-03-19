elasticsearch-keystore-ready:
    test.nop

create-keystore:
    cmd.run:
      - name: /usr/share/elasticsearch/bin/elasticsearch-keystore create
      - cwd: /etc/elasticsearch
      - creates: /etc/elasticsearch/elasticsearch.keystore
      - require_in: 
          - test: elasticsearch-keystore-ready

ensure-keystore:
    mdb_elasticsearch.ensure_keystore:
      - settings: {{ salt.mdb_elasticsearch.keystore_settings() }}
      - require:
          - cmd: create-keystore
      - require_in: 
          - test: elasticsearch-keystore-ready

/etc/elasticsearch/elasticsearch.keystore:
    file.managed:
      - replace: False
      - mode: 640
      - user: elasticsearch
      - group: elasticsearch
      - require:
          - cmd: create-keystore
          - mdb_elasticsearch: ensure-keystore
      - require_in: 
          - test: elasticsearch-keystore-ready

reload-keystore:
    module.run:
      - name: mdb_elasticsearch.reload_secure_settings
      - require:
          - file: /etc/elasticsearch/elasticsearch.keystore
      - onchanges:
          - mdb_elasticsearch: ensure-keystore
