elasticsearch-extensions-ready:
    test.nop

exts-always-passes:
    test.nop

/etc/elasticsearch/extensions:
    file.directory:
        - user: elasticsearch
        - group: elasticsearch
        - file_mode: 600
        - dir_mode: 700
        - clean: True
        - require:
            - test: exts-always-passes
{% for ext in salt.mdb_elasticsearch.extensions() %}
            - file: /etc/elasticsearch/extensions/{{ ext.name }}
{% endfor %}

/var/lib/elasticsearch/.mdb/exts:
    file.directory:
        - user: elasticsearch
        - group: elasticsearch
        - file_mode: 600
        - dir_mode: 700
        - clean: True
        - makedirs: True
        - require:
            - test: exts-always-passes
{% for ext in salt.mdb_elasticsearch.extensions() %}
            - file: /var/lib/elasticsearch/.mdb/exts/{{ ext.id }}
{% endfor %}

/var/lib/elasticsearch/.mdb/exts_arc:
    file.directory:
        - user: root
        - group: elasticsearch
        - file_mode: 600
        - dir_mode: 700
        - clean: True
        - makedirs: True
        - require:
            - test: exts-always-passes
{% for ext in salt.mdb_elasticsearch.extensions() %}
            - file: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip
{% endfor %}

{% for ext in salt.mdb_elasticsearch.extensions() %}
/var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip:
    fs.file_present:
        - s3_cache_path: "{{ ext.path }}"
        - url: "{{ ext.uri }}"
        - use_service_account_authorization: {{ ext.use_service_account }}
        - skip_verify: True
        - user: root
        - group: elasticsearch
        - mode: 640
        - makedirs: True
        - replace: False
        - s3_bucket: {{ salt.mdb_elasticsearch.s3bucket() }}
    file.exists:
        - require:
            - fs: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip

/var/lib/elasticsearch/.mdb/exts/{{ ext.id }}:
    archive.extracted:
        - source: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip
        - enforce_toplevel: False
        - user: elasticsearch
        - group: elasticsearch
        - require:
            - file: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip
    file.exists:
        - require:
            - archive: /var/lib/elasticsearch/.mdb/exts/{{ ext.id }}

/etc/elasticsearch/extensions/{{ ext.name }}:
    file.symlink:
        - target: /var/lib/elasticsearch/.mdb/exts/{{ ext.id }}
        - force: True
        - makedirs: True
        - user: elasticsearch
        - group: elasticsearch
        - file_mode: 600
        - dir_mode: 700
        - require:
            - file: /var/lib/elasticsearch/.mdb/exts/{{ ext.id }} 
        - require_in:
            - test: elasticsearch-extensions-ready
{% endfor %}
