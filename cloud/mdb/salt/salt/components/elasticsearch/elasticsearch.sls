{% set elasticsearch_version = salt.mdb_elasticsearch.version() %}
{% set plugins_version = salt.mdb_elasticsearch.plugins_version() %}

# OSS vs Licensed package
{% set package_name = 'elasticsearch' if salt.mdb_elasticsearch.licensed_for('basic') else 'elasticsearch-oss' %}

elasticsearch-packages:
    pkg.installed:
        - pkgs:
            - {{package_name}}: {{ elasticsearch_version }}
            - mdb-elasticsearch-plugins: {{ plugins_version }}
            - python-bcrypt
            - python3-bcrypt
            - python-magic
            - python3-magic
            - python-boto3
            - python3-boto3

# fix post remove script for switching between oos and licensed versions
/var/lib/dpkg/info/{{package_name}}.postrm:
  file.replace:
    - pattern: 'rmdir --ignore-fail-on-non-empty /var/lib/elasticsearch'
    - repl: 'echo skip rmdir /var/lib/elasticsearch # --ignore-fail-on-non-empty /var/lib/elasticsearch'
    - require:
            - pkg: elasticsearch-packages

# just for add es-certs group
elasticsearch-user:
    user.present:
        - name: elasticsearch
        - groups:
            - elasticsearch
            - es-certs
        - require_in:
            - test: elasticsearch-service-req
        - require:
            - pkg: elasticsearch-packages
            - group: es-certs

/var/lib/elasticsearch:
    file.directory:
        - user: elasticsearch
        - group: elasticsearch
        - mode: 755
        - makedirs: True
        - require_in:
            - test: elasticsearch-service-req
        - require:
            - pkg: elasticsearch-packages

/var/log/elasticsearch:
    file.directory:
        - user: elasticsearch
        - group: elasticsearch
        - mode: 775
        - makedirs: True
        - require_in:
            - test: elasticsearch-service-req
        - require:
            - pkg: elasticsearch-packages

/etc/elasticsearch:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - makedirs: True
        - require_in:
            - test: elasticsearch-service-req
        - require:
            - pkg: elasticsearch-packages

/etc/systemd/system/elasticsearch.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: elasticsearch-packages
        - require_in:
            - test: elasticsearch-service-req
        - source: salt://{{ slspath }}/conf/elasticsearch.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/elasticsearch.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: elasticsearch-packages
        - require_in:
            - test: elasticsearch-service-req
        - source: salt://{{ slspath }}/conf/elasticsearch.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/es_cli.py:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/es_cli.py
        - mode: 750
        - makedirs: True

elasticsearch-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # alias for elastic cli tool
            alias elastic='/usr/local/yandex/es_cli.py'
        - require_in:
            - file: /root/.bashrc

/etc/elasticsearch/jvm.options.d:
    file.directory:
        - user: elasticsearch
        - group: elasticsearch
        - mode: 755
        - makedirs: True
        - require_in:
            - test: elasticsearch-service-req

/etc/elasticsearch/jvm.options.d/heap.options:
    file.absent

/etc/elasticsearch/es_heap.py:
    file.absent

include:
    - .certs
    - .elasticsearch-config
    - .elasticsearch-keystore
    - .elasticsearch-plugins
    - .elasticsearch-service
    - .users
    - .backups
    - .hunspell
    - components.common.systemd
{% if salt.pillar.get('restore_from')%}
    - .backups.restore
{% endif %}

extend:
    elasticsearch-service-req:
        test.nop:
            - require:
                - test: elasticsearch-conf-ready
                - test: elasticsearch-keystore-ready
                - test: elasticsearch-plugins-ready
                - test: elasticsearch-sync-users-ready
                - test: elasticsearch-hunspell-ready
                - test: certs-ready

    elasticsearch-backups-req:
        test.nop:
            - require:
                - test: elasticsearch-service-ready

    backups-user:
        user.present:
            - require:
                - pkg: elasticsearch-packages
                - test: certs-ready

    create-keystore:
        cmd.run:
            - require:
                - pkg: elasticsearch-packages

    ensure-keystore:
        mdb_elasticsearch.ensure_keystore:
            - require:
                - pkg: elasticsearch-packages

    reload-keystore:
        module.run:
            - require:
                - module: elasticsearch-service

    backups-repository:
        mdb_elasticsearch.ensure_repository:
            - require:
                - module: reload-keystore

    elasticsearch-plugins:
        mdb_elasticsearch.ensure_plugins:
            - require:
                - pkg: elasticsearch-packages

    /etc/elasticsearch/elasticsearch.yml:
        file.managed:
            - require:
                - pkg: elasticsearch-packages

    /etc/elasticsearch/jvm.options:
        file.managed:
            - require:
                - pkg: elasticsearch-packages

    /etc/default/elasticsearch:
        file.managed:
            - require:
                - pkg: elasticsearch-packages

    /etc/elasticsearch/log4j2.properties:
        file.managed:
            - require:
                - pkg: elasticsearch-packages

    elasticsearch-sync-users-req:
        test.nop:
            - require:
                - pkg: elasticsearch-packages
