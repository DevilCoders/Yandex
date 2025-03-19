/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-rd-config
        - mode: '0644'
        - require_in:
            - pkg: redis-security-pkgs

old-redis-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-rd-config
        - require_in:
            - file: /etc/osquery.tag

redis-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-rd-prof: 1.2.19
        - require:
            - pkg: apparmor
            - pkg: old-redis-security-pkgs

apparmor-enforce:
    cmd.wait:
        - name: 'aa-enforce /usr/bin/redis-check-rdb'
        - watch:
            - pkg: redis-security-pkgs
