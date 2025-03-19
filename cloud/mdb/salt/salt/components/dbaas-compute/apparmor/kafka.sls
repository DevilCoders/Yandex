/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-kafka
        - mode: '0644'
        - require_in:
            - pkg: kafka-security-pkgs

old-kafka-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-kafka-config
        - require_in:
            - file: /etc/osquery.tag

kafka-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-kafka-prof: 0.1.7
        - require:
            - pkg: apparmor
            - pkg: old-kafka-security-pkgs

apparmor-kafka-complain:
    cmd.wait:
        - name: 'aa-complain /opt/kafka/bin/kafka-server-start.sh'
        - watch:
            - pkg: kafka-security-pkgs
