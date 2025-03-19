/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-elastic
        - mode: '0644'
        - require_in:
            - pkg: elasticsearch-security-pkgs

old-elasticsearch-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-elastic-config
        - require_in:
            - file: /etc/osquery.tag

elasticsearch-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-elastic-prof: 1.0.7
        - require:
            - pkg: apparmor
            - pkg: old-elasticsearch-security-pkgs

apparmor-complain:
    cmd.wait:
        - name: 'aa-complain /etc/apparmor.d/usr.share.elasticsearch.bin.elasticsearch'
        - watch:
            - pkg: elasticsearch-security-pkgs
