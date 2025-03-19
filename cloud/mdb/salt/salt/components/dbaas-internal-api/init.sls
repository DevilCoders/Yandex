# Supervisor and nginx should be already installed
# Not adding it because of conflicts on initial deploy
{% set log_dir = '/var/log/dbaas-internal-api' %}
{% set use_yasmagent = salt['pillar.get']('data:use_yasmagent', True) %}
{% set use_pushclient = salt['pillar.get']('data:use_pushclient', False) %}
{% set use_monrun2 = salt['pillar.get']('data:monrun2', False) %}
{% set use_mdbmetrics = salt['pillar.get']('data:use_mdbmetrics', True) %}
{% set use_search_reindexer = salt['pillar.get']('data:search-reindexer:service_account:key_id') %}
{% set osrelease = salt['grains.get']('osrelease') %}

{% if use_yasmagent or use_pushclient or use_monrun2 or use_mdbmetrics or use_search_reindexer %}
include:
{% if use_monrun2 %}
    - components.monrun2.http-ping
{% endif %}
{% if use_yasmagent %}
    - .yasmagent
{% endif %}
{% if use_pushclient %}
    - components.pushclient2
{% endif %}
{% if use_mdbmetrics %}
    - .mdb-metrics
{% endif %}
{% if use_search_reindexer %}
    - .search-reindexer
{% endif %}
{% endif %}

dbaas-internal-api-pkgs:
    pkg.installed:
        - pkgs:
            - dbaas-internal-api: '2.9761135'

/opt/yandex/dbaas-internal-api/config.py:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/config.py' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - require:
            - pkg: dbaas-internal-api-pkgs

internal-api-supervised:
    supervisord.running:
        - name: internal_api
        - update: True
        - require:
            - service: supervisor-service
            - user: web-api-user
            - file: /var/log/dbaas-internal-api
        - watch:
            - pkg: dbaas-internal-api-pkgs
            - file: /opt/yandex/dbaas-internal-api/config.py
            - file: /etc/supervisor/conf.d/internal_api.conf

/etc/supervisor/conf.d/internal_api.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/internal_api.conf
        - require:
            - pkg: dbaas-internal-api-pkgs

/etc/nginx/ssl/dbaas-internal-api.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/dbaas-internal-api.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/dbaas-internal-api.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - pkg: dbaas-internal-api-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/var/www/env/dbaas_internal_api:
    file.directory:
        - user: root
        - group: root
        - dir_mode: 755
        - makedirs: True
        - require:
            - pkg: nginx-packages

/var/www/env/dbaas_internal_api/YandexCLCA.pem:
    file.managed:
        - source: salt://{{ slspath }}/conf/YandexCLCA.pem
        - user: root
        - group: root
        - mode: 644

{{ log_dir }}:
    file.directory:
        - user: web-api
        - group: web-api
        - dir_mode: 750
        - require:
            - user: web-api-user

{% if use_pushclient %}
/etc/pushclient/conf.d/topics-logs-grpc.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/pushclient-topics-logs-grpc.conf
        - template: jinja
        - makedirs: True
        - require:
            - pkg: pushclient
        - watch_in:
            - service: pushclient
        - require_in:
            - file: /etc/pushclient/push-client-grpc.conf
{% endif %}

statbox-in-web-api-group:
    group.present:
        - name: web-api
        - system: True
{% if use_pushclient %}
        - require:
            - user: statbox-user
        - addusers:
            - statbox
        - watch_in:
            - service: pushclient
        - require_in:
            - service: pushclient
{% endif %}

monitor-in-web-api-group:
    group.present:
        - name: web-api
        - addusers:
            - monitor
        - system: True
{% if use_pushclient %}
        - require_in:
            - service: pushclient
{% endif %}
{% if salt['pillar.get']('data:monrun2', False) %}
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: juggler-client-restart
{% endif %}
