include:
    - components.tvmtool

blackbox-sso-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-blackbox-sso: '1.7352387'

/etc/blackbox-sso.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/blackbox-sso.yaml' }}
        - mode: '0640'
        - require:
            - pkg: blackbox-sso-pkgs

/etc/supervisor/conf.d/blackbox-sso.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: blackbox-sso-pkgs
            - service: supervisor-service

blackbox-sso-supervised:
    supervisord.running:
        - name: blackbox-sso
        - update: True
        - require:
            - service: supervisor-service
        - watch:
            - pkg: blackbox-sso-pkgs
            - file: /etc/supervisor/conf.d/blackbox-sso.conf
            - file: /etc/blackbox-sso.yaml
