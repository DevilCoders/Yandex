dbaas-cleaner-pkgs:
    pkg.installed:
        - pkgs:
            - dbaas-cleaner: '1.8225529'

/opt/yandex/dbaas-cleaner/config.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/config.yaml
        - mode: 644
        - require:
            - pkg: dbaas-cleaner-pkgs

/opt/yandex/dbaas-cleaner/CA.pem:
    file.managed:
        - source: https://crls.yandex.net/allCAs.pem
        - skip_verify: True
        - mode: 644
        - require:
            - pkg: dbaas-cleaner-pkgs

/etc/cron.d/dbaas-cleaner:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbaas-cleaner.cron
        - mode: 644
        - require:
            - pkg: dbaas-cleaner-pkgs

/etc/logrotate.d/dbaas-cleaner:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - require:
            - pkg: dbaas-cleaner-pkgs
