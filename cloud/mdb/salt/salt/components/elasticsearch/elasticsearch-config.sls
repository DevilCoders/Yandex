elasticsearch-conf-ready:
    test.nop

/etc/elasticsearch/elasticsearch.yml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/elasticsearch.yml
        - makedirs: True
        - mode: 644
        - require_in:
            - test: elasticsearch-conf-ready

/etc/elasticsearch/jvm.options:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/jvm.options
        - makedirs: True
        - mode: 644
        - require_in:
            - test: elasticsearch-conf-ready

/etc/default/elasticsearch:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/elasticsearch.env.sh
        - makedirs: True
        - mode: 644
        - require_in:
            - test: elasticsearch-conf-ready

/etc/elasticsearch/log4j2.properties:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/log4j2.properties
        - makedirs: True
        - mode: 644
        - require_in:
            - test: elasticsearch-conf-ready

auth-files-dir:
    file.directory:
        - name: /etc/elasticsearch/auth_files
        - user: elasticsearch
        - group: elasticsearch
        - file_mode: 664
        - dir_mode: 755

auth-files-clean:
    file.directory:
        - name: /etc/elasticsearch/auth_files
        - clean: True
        - require:
            - file: auth-files-dir
{% for fname, content_key in salt.mdb_elasticsearch.auth_files() %}
            - file: /etc/elasticsearch/auth_files/{{ fname }}
{% endfor %}

{% for fname, content_key in salt.mdb_elasticsearch.auth_files() %}
/etc/elasticsearch/auth_files/{{ fname }}:
    file.decode:
        - contents_pillar: {{ content_key }}
        - require:
            - file: auth-files-dir
        - require_in:
            - test: elasticsearch-conf-ready
{% endfor %}