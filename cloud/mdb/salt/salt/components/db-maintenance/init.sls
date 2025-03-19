dbm-packages:
    pkg.installed:
        - pkgs:
            - mdb-dbm: '1.9324196'

/etc/yandex/mdb-dbm:
    file.directory:
        - makedirs: True
        - require:
             - pkg: dbm-packages

/etc/yandex/mdb-dbm/app.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/app.yaml
        - mode: 644
        - user: root
        - group: root
        - require:
            - file: /etc/yandex/mdb-dbm

/etc/yandex/mdb-dbm/uwsgi.ini:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/uwsgi.ini
        - mode: 644
        - user: root
        - group: root
        - require:
            - file: /etc/yandex/mdb-dbm

/var/log/db-maintenance:
    file.directory:
        - mode: 750
        - user: web-api
        - group: root
        - require:
            - pkg: dbm-packages
        - require_in:
            - service: mdb-dbm-service

/etc/nginx/conf.d/db_maintenance.conf:
    file.managed:
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/nginx_db_maintenance.conf
        - watch_in:
            - service: nginx-service

/lib/systemd/system/mdb-dbm.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-dbm.service
        - mode: 644
        - require:
            - pkg: dbm-packages
        - onchanges_in:
            - module: systemd-reload

mdb-dbm-service:
    service.running:
        - name: mdb-dbm
        - enable: True
        - watch:
            - pkg: dbm-packages
            - file: /lib/systemd/system/mdb-dbm.service
            - file: /etc/yandex/mdb-dbm/app.yaml
            - file: /etc/yandex/mdb-dbm/uwsgi.ini

/etc/cron.d/dbm-expire-secrets:
    file.absent

/etc/nginx/ssl/dhparam.pem:
    file.managed:
        - user: www-data
        - group: root
        - mode: 640
        - makedirs: True
        - dir_mode: 755
        - contents_pillar: data:common:dh
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/db_maintenance.pem:
    file.managed:
        - user: root
        - group: www-data
        - mode: 640
        - makedirs: True
        - dir_mode: 755
{% if salt['pillar.get']('data:pg_ssl_balancer') %}
        - contents_pillar: cert.crt
{% else %}
        - contents_pillar: data:db_maintenance:ssl:cert
{% endif %}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/db_maintenance.key:
    file.managed:
        - user: root
        - group: www-data
        - mode: 640
        - makedirs: True
        - dir_mode: 755
{% if salt['pillar.get']('data:pg_ssl_balancer') %}
        - contents_pillar: cert.key
{% else %}
        - contents_pillar: data:db_maintenance:ssl:key
{% endif %}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service
