mdb-mlock-cli-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-mlock-cli: '1.9404383'
        - prereq_in:
            - cmd: repositories-ready

/root/.mlock-cli.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mlock-cli.yaml' }}
        - mode: '0600'
        - user: root
        - group: root

/home/monitor/.mlock-cli.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mlock-cli.yaml' }}
        - mode: '0600'
        - user: monitor
        - group: monitor
        - require:
            - pkg: juggler-pgks
