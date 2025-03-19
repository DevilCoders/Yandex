opensearch-configuration:
    file.recurse:
        - name: /etc/opensearch
        - source: salt://{{ slspath }}/conf/etc/opensearch
        - template: jinja
        - user: root
        - group: opensearch
        - dir_mode: 755
        - file_mode: 640
        - recurse:
            - user
            - group
            - mode
        - include_empty: true
        - clean: true
        - exclude_pat: E@(certs.*)|(opensearch.keystore)|(.opensearch.keystore.initial_md5sum)|(repository-s3.*)|(hunspell.*)
