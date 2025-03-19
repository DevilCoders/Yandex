mdb-ui-packages:
    pkg.installed:
        - pkgs:
            - mdb-ui: '65-ee798c4'
        - prereq_in:
            - cmd: repositories-ready

/var/www/mdbui/config.ini:
    file.absent

/var/www/mdbui/config.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/config.yaml
        - mode: 640
        - user: root
        - group: web-api
        - makedirs: True
        - require:
            - pkg: mdb-ui-packages
        - watch_in:
            - service: mdbui-service

/var/log/mdb-ui:
    file.absent

/etc/nginx/conf.d/mdbui.conf:
    file.managed:
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/nginx_mdbui.conf
        - watch_in:
            - service: nginx-service

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

/etc/nginx/ssl/mdbui.pem:
    file.managed:
        - user: root
        - group: www-data
        - mode: 640
        - makedirs: True
        - dir_mode: 755
{% if salt['pillar.get']('data:pg_ssl_balancer') %}
        - contents_pillar: cert.crt
{% else %}
        - contents_pillar: data:mdbui:ssl:cert
{% endif %}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdbui.key:
    file.managed:
        - user: root
        - group: www-data
        - mode: 640
        - makedirs: True
        - dir_mode: 755
{% if salt['pillar.get']('data:pg_ssl_balancer') %}
        - contents_pillar: cert.key
{% else %}
        - contents_pillar: data:mdbui:ssl:key
{% endif %}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/lib/systemd/system/mdb-ui.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-ui.service
        - require:
            - pkg: mdb-ui-packages
        - onchanges_in:
            - module: systemd-reload

/var/log/mdbui:
    file.directory:
        - user: web-api
        - group: web-api

mdbui-service:
    service:
        - running
        - enable: true
        - name: mdb-ui
        - require:
            - file: /var/log/mdbui
        - watch:
            - pkg: mdb-ui-packages

/etc/logrotate.d/mdbui:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdbui-logrotate
        - user: root
        - group: root
        - mode: 644

reload-after-update-pkg:
    cmd.run:
        - name: >
            cd /var/www/mdbui &&
            venv/bin/python manage.py collectstatic --noinput &&
            venv/bin/python manage.py migrate --noinput &&
            touch uwsgi
        - onchanges:
            - pkg: mdb-ui-packages
