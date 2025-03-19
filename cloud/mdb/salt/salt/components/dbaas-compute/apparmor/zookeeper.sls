{% set working_with_kafka = salt.pillar.get('data:zk:apparmor_disabled', False) %}
{% if not working_with_kafka %}
/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-zookeeper-config
        - mode: '0644'
        - require_in:
            - pkg: zookeeper-security-pkgs
{% endif %}

old-zookeeper-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-zookeeper-config
        - require_in:
            - file: /etc/osquery.tag

zookeeper-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-zookeeper-prof: 1.0.24
        - require:
            - pkg: apparmor
            - pkg: old-zookeeper-security-pkgs

apparmor-complain:
    cmd.wait:
        - name: 'aa-complain /usr/lib/jvm/java-11-openjdk-amd64/bin/java-zookeeper'
        - watch:
            - pkg: zookeeper-security-pkgs
