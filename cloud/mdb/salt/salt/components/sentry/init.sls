include:
    - components.web-api-base

sentry-pkgs:
    pkg.installed:
        - pkgs:
            - dbaas-sentry: '9.1.2-9a7b130'

/etc/sentry/sentry.conf.py:
    file.managed:
        - makedirs: True
        - template: jinja
        - mode: '0640'
        - user: root
        - group: www-data
        - source: salt://{{ slspath }}/conf/sentry.conf.py
        - require:
            - user: web-api-user

/etc/sentry/config.yml:
    file.managed:
        - makedirs: True
        - mode: '0640'
        - user: root
        - group: www-data
        - source: ~
        - require:
            - user: web-api-user

/etc/supervisor/conf.d/sentry.conf:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/sentry.conf.supervisor

/etc/nginx/ssl/sentry.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/sentry.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/sentry.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/nginx.conf
        - require:
            - pkg: sentry-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

sentry-web-supervised:
    supervisord.running:
        - name: sentry
        - update: True
        - require:
            - service: supervisor-service
            - user: web-api-user
        - watch:
            - pkg: sentry-pkgs
            - file: /etc/sentry/config.yml
            - file: /etc/sentry/sentry.conf.py
            - file: /etc/supervisor/conf.d/sentry.conf

sentry-cron-supervised:
    supervisord.running:
        - name: sentry-cron
        - update: True
        - require:
            - service: supervisor-service
            - user: web-api-user
        - watch:
            - pkg: sentry-pkgs
            - file: /etc/sentry/config.yml
            - file: /etc/sentry/sentry.conf.py
            - file: /etc/supervisor/conf.d/sentry.conf

sentry-worker-supervised:
    supervisord.running:
        - name: sentry-worker
        - update: True
        - require:
            - service: supervisor-service
            - user: web-api-user
        - watch:
            - pkg: sentry-pkgs
            - file: /etc/sentry/config.yml
            - file: /etc/sentry/sentry.conf.py
            - file: /etc/supervisor/conf.d/sentry.conf

/etc/cron.d/sentry-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/sentry-cleanup.cron

/etc/logrotate.d/sentry-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/sentry-cleanup.logrotate

/etc/yandex/dbaas_sentry_reconfig.sh:
    file.absent
