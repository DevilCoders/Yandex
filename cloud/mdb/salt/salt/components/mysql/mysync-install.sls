/etc/systemd/system/mysync.service:
    file.absent

/lib/systemd/system/mysync.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mysync.service
        - mode: 644
        - require:
            - test: mysync-pre-install
            - pkg: mysync-pkg
        - require_in:
            - test: mysync-installed
        - onchanges_in:
            - module: systemd-reload

/var/run/mysync:
    file.directory:
        - user: mysql
        - group: mysql
        - mode: 0755
        - makedirs: True
        - require:
            - test: mysync-pre-install
        - require_in:
            - test: mysync-installed

/var/log/mysync:
    file.directory:
        - user: mysql
        - group: mysql
        - mode: 755
        - makedirs: True
        - require:
            - test: mysync-pre-install
        - require_in:
            - test: mysync-installed

mysync-pkg:
    pkg.installed:
        - pkgs:
            - mdb-mysync: '1.9655029'
        - require:
            - test: mysync-pre-install
        - require_in:
            - test: mysync-installed

mysync-pre-install:
    test.nop

mysync-installed:
    test.nop
