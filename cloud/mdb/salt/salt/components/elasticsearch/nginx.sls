# common certs between nginx elastic and kibana in /etc/elasticsearch/certs
/etc/nginx/ssl:
    file.absent

include:
    - .certs

nginx-packages:
    pkg.installed:
        - pkgs:
            - nginx: 1.14.2-1.yandex.6

nginx-service:
    service:
        - running
        - enable: true
        - reload: true
        - name: nginx
        - require:
            - test: certs-ready
        - watch:
            - pkg: nginx-packages
            - file: /etc/elasticsearch/certs/server.key
            - file: /etc/elasticsearch/certs/server.crt

# just for add es-certs group
nginx-user:
    user.present:
        - name: www-data
        - groups:
            - www-data
            - es-certs
        - require_in:
            - service: nginx-service
        - require:
            - pkg: nginx-packages
            - group: es-certs

/etc/logrotate.d/nginx:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-logrotate

/etc/nginx/nginx.conf:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx.conf
        - watch_in:
            - service: nginx-service

/etc/nginx/conf.d/kibana.conf:
{% if salt.pillar.get('data:elasticsearch:kibana:enabled', False) %}
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-kibana.conf
        - watch_in:
            - service: nginx-service
{% else %}
    file.absent
{% endif %}

/etc/nginx/conf.d/elasticsearch.conf:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/nginx-es.conf
        - watch_in:
            - service: nginx-service
