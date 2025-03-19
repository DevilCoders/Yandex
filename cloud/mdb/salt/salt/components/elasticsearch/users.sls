elasticsearch-sync-users-req:
    test.nop

elasticsearch-sync-users-ready:
    test.nop

{% if salt.mdb_elasticsearch.licensed_for('basic') %}

/etc/elasticsearch/roles.yml:
    file.managed:
        - user: elasticsearch
        - group: elasticsearch
        - mode: 640
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/roles.yml
        - require:
            - test: elasticsearch-sync-users-req
        - require_in:
            - test: elasticsearch-sync-users-ready

/etc/elasticsearch/operator_users.yml:
{% if salt.mdb_elasticsearch.version_ge('7.11') %}
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/operator_users.yml
        - makedirs: True
        - user: root
        - group: elasticsearch
        - mode: 640
        - require:
            - test: elasticsearch-sync-users-req
        - require_in:
            - test: elasticsearch-sync-users-ready
{% else %}
    file.absent
{% endif %}

users-file:
    fs.file_present:
        - name: /etc/elasticsearch/users
        - contents_function: mdb_elasticsearch.render_users_file
        - user: root
        - group: elasticsearch
        - mode: 640
        - makedirs: True
        - require:
            - test: elasticsearch-sync-users-req
        - require_in:
            - test: elasticsearch-sync-users-ready

users-roles-file:
    fs.file_present:
        - name: /etc/elasticsearch/users_roles
        - contents_function: mdb_elasticsearch.render_users_roles_file
        - user: root
        - group: elasticsearch
        - mode: 640
        - makedirs: True
        - require:
            - test: elasticsearch-sync-users-req
        - require:
              - file: /etc/elasticsearch/roles.yml
        - require_in:
            - test: elasticsearch-sync-users-ready

nginx-remove-basic-auth:
    file.absent:
        - name: /etc/nginx/conf.d/basic-auth.conf
        - watch_in:
            - service: nginx-service

nginx-remove-htpasswd:
    file.absent:
        - name: /etc/nginx/htpasswd
        - watch_in:
            - service: nginx-service

{% else %}

/etc/nginx/conf.d/basic-auth.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-auth.conf
        - watch_in:
            - service: nginx-service

/etc/nginx/htpasswd:
    fs.file_present:
        - contents_function: mdb_elasticsearch.render_htpasswd
        - user: root
        - group: www-data
        - mode: 640
        - makedirs: True
        - watch_in:
            - service: nginx-service
{% endif %}
