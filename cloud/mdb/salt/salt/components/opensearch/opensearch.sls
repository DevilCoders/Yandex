# TODO version mapping
# TODO backups

#################################
### Install Required Packages

opensearch-packages:
    pkg.installed:
        - pkgs:
            - opensearch: 2.1.0
            - python-bcrypt
            - python3-bcrypt
            - python-magic
            - python3-magic
            - python-boto3
            - python3-boto3
        - require_in:
            - test: opensearch-service-req
            - file: opensearch-configuration
            - mdb_opensearch: opensearch-plugins
            - test: opensearch-keystore-req

#################################
### User and Group

opensearch-group:
    group.present:
        - name: opensearch

# just for add es-certs group
opensearch-user:
    user.present:
        - name: opensearch
        - groups:
            - opensearch
            - es-certs
        - require_in:
            - test: opensearch-service-req
        - require:
            - pkg: opensearch-packages
            - group: opensearch-group
            - group: es-certs

#################################
### Tools

opensearch-tools:
    file.recurse:
        - name: /usr/local/yandex
        - source: salt://{{ slspath }}/conf/usr/local/yandex
        - template: jinja
        - user: root
        - group: root
        - dir_mode: 755
        - file_mode: 750
        - recurse:
            - user
            - group
            - mode
        - include_empty: true
        - require_in:
            - test: opensearch-service-req
            - test: hunspell-req

/var/lib/opensearch:
    file.directory:
        - user: opensearch
        - group: opensearch
        - mode: 755
        - makedirs: True
        - require_in:
            - test: opensearch-service-req
        - require:
            - pkg: opensearch-packages

/var/log/opensearch:
    file.directory:
        - user: opensearch
        - group: opensearch
        - mode: 775
        - makedirs: True
        - require_in:
            - test: opensearch-service-req
        - require:
            - pkg: opensearch-packages

/etc/systemd/system/opensearch.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: opensearch-packages
        - require_in:
            - test: opensearch-service-req
        - source: salt://{{ slspath }}/conf/opensearch.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

elasticsearch-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # alias for elastic cli tool
            alias elastic='/usr/local/yandex/es_cli.py'
        - require_in:
            - file: /root/.bashrc

include:
    - components.common.systemd-reload
    - .certs
    - .plugins
    - .config
    - .keystore
    - .service
    - .hunspell

extend:
    opensearch-configuration:
        file.recurse:
            - require:
                - mdb_opensearch: opensearch-plugins

    reload-keystore:
        module.run:
            - require:
                - module: opensearch-service

    opensearch-service-req:
        test.nop:
            - require:
                - test: opensearch-keystore-ready
                - test: certs-ready
                - file: opensearch-configuration
                - mdb_opensearch: opensearch-plugins
