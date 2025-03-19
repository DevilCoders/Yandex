{% if salt['pillar.get']('data:web_api_base:use_nginx', True) %}
/etc/nginx/ssl/allCAs.pem:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - user: root
        - group: www-data
        - makedirs: True
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service
        - require:
            - file: /etc/nginx/ssl
            - file: /opt/yandex/allCAs.pem

/etc/nginx/ssl:
    file.directory:
        - user: root
        - group: www-data
        - mode: 0755
        - makedirs: True

nginx-packages:
    pkg.installed:
        - pkgs:
            - nginx: 1.14.2-1.yandex.6

nginx-service:
    service:
        - running
        - enable: true
        - name: nginx
        - watch:
            - pkg: nginx-packages

/etc/nginx/nginx.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx.conf
        - watch_in:
            - service: nginx-service

/etc/logrotate.d/nginx:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-logrotate
{% else %}
/etc/nginx:
    file.absent

nginx-packages:
    pkg.purged

/etc/logrotate.d/nginx:
    file.absent
{% endif %}

web-api-group:
    group.present:
        - name: web-api
        - system: True

web-api-user:
    user.present:
        - fullname: HTTP API system user
        - name: web-api
        - gid_from_name: True
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True
        - groups:
            - www-data
        - require:
            - group: web-api

{% set cert_ca = salt['pillar.get']('cert.ca') %}
/home/web-api/.postgresql/root.crt:
    file.symlink:
{% if cert_ca %}
        - target: /opt/yandex/CA.pem
{% else %}
        - target: /opt/yandex/allCAs.pem
{% endif %}
        - force: True
        - user: web-api
        - group: web-api
        - makedirs: True
        - require:
            - user: web-api-user

/root/.postgresql/root.crt:
    file.symlink:
{% if cert_ca %}
        - target: /opt/yandex/CA.pem
{% else %}
        - target: /opt/yandex/allCAs.pem
{% endif %}
        - force: True
        - user: root
        - group: root
        - makedirs: True
