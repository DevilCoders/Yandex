include:
    - .mdb-metrics
    - components.mdb-mlock-cli
    - components.monrun2.mdb-mlock

mdb-mlock-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-mlock: '1.9404383'
        - require:
            - cmd: repositories-ready

mdb-mlock-group:
    group.present:
        - name: mlock
        - system: True

mdb-mlock-user:
    user.present:
        - name: mlock
        - gid_from_name: True
        - createhome: False
        - empty_password: True
        - shell: /usr/sbin/nologin
        - system: True
        - require:
            - group: mdb-mlock-group

/etc/mlock.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mlock.yaml' }}
        - mode: '0640'
        - user: mlock
        - group: mlock
        - require:
            - user: mdb-mlock-user

/etc/systemd/system/mdb-mlock.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mlock.service
        - onchanges_in:
            - module: systemd-reload
        - require:
            - pkg: mdb-mlock-pkgs
            - user: mdb-mlock-user

mdb-mlock-service:
    service.running:
        - name: mdb-mlock
        - enable: True
        - watch:
            - pkg: mdb-mlock-pkgs
            - file: /etc/mlock.yaml
            - file: /etc/systemd/system/mdb-mlock.service
