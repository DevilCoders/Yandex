elasticsearch-plugins-ready:
    test.nop

elasticsearch-plugins:
    mdb_elasticsearch.ensure_plugins:
      - require_in: 
          - test: elasticsearch-plugins-ready


/etc/elasticsearch/repository-s3:
  file.directory:
    - user: elasticsearch
    - group: elasticsearch
    - file_mode: 660
    - dir_mode: 750
    - makedirs: False
    - recurse:
      - user
      - group
      - mode
    - require:
      - mdb_elasticsearch: elasticsearch-plugins
    - require_in: 
      - test: elasticsearch-plugins-ready

/etc/elasticsearch/discovery-ec2:
  file.directory:
    - user: elasticsearch
    - group: elasticsearch
    - file_mode: 660
    - dir_mode: 750
    - makedirs: False
    - recurse:
      - user
      - group
      - mode
    - require:
      - mdb_elasticsearch: elasticsearch-plugins
    - require_in: 
      - test: elasticsearch-plugins-ready
