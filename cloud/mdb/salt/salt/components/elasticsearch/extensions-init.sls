{% for ext in salt.mdb_elasticsearch.new_extensions() %}
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
    module.run:
        - name: mdb_elasticsearch.validate_extension
        - archive: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip
        - extname: {{ ext.name }}
        - require:
            - fs: /var/lib/elasticsearch/.mdb/exts_arc/{{ ext.id }}.zip
{% endfor %}
