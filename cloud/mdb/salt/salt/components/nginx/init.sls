{% set nginx_version = '1.14.2-1.yandex.6' %}

include:
    - components.logrotate
    - .mdb-metrics

nginx-packages:
    pkg.installed:
        - pkgs:
            - nginx: {{ nginx_version }}

nginx-service:
    service:
        - running
        - enable: true
        - name: nginx
        - watch:
            - pkg: nginx-packages

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

/etc/nginx/ssl/allCAs.pem:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - user: root
        - group: www-data
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

/etc/nginx/nginx.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx.conf
        - watch_in:
            - service: nginx-service

/etc/nginx/includes:
    file.recurse:
        - template: jinja
        - source: salt://{{ slspath }}/includes
        - watch_in:
            - service: nginx-service

/etc/nginx/lua:
    file.recurse:
        - template: jinja
        - source: salt://{{ slspath }}/lua
        - watch_in:
            - service: nginx-service

/etc/logrotate.d/nginx:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-logrotate
        - mode: 0444
