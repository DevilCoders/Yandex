include:
    - components.monrun2.http-ping
    - components.monrun2.http-tls

/home/web-api/.pgpass:
    file.managed:
        - user: web-api
        - group: web-api
        - mode: 600
        - makedirs: True
        - dir_mode: 750
        - contents: |
            *:*:dbaas_metadb:idm_service:{{ salt['pillar.get']('data:config:pgusers:idm_service:password') }}
        - require:
            - user: web-api-user

idm-api-packages:
    pkg.installed:
        - pkgs:
            - yandex-mdb-idm-service: '1.9765570'

/opt/yandex/mdb-idm-service/config.py:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/config.py
        - require:
            - pkg: idm-api-packages

/etc/supervisor/conf.d/idm_service.conf:
    file.managed:
        - makedirs: True
        - source: salt://{{ slspath }}/conf/idm_service.conf

idm-service-supervised:
    supervisord.running:
        - name: mdb_idm_service
        - update: True
        - require:
            - service: supervisor-service
            - user: web-api-user
        - watch:
            - pkg: idm-api-packages
            - file: /opt/yandex/mdb-idm-service/config.py
            - file: /etc/supervisor/conf.d/idm_service.conf

/etc/nginx/ssl/mdb_idm.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb_idm.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/mdb_idm.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/idm_nginx.conf' }}
        - require:
            - pkg: idm-api-packages
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/cron.d/rotate_idm_passwords:
    file.managed:
        - source: salt://{{slspath}}/conf/rotate_idm_passwords.cron
        - mode: 644
