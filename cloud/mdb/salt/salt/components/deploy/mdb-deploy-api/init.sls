# Supervisor and nginx should be already installed
# Not adding it because of conflicts on initial deploy

include:
{% if salt.pillar.get('data:install_tvm', True) %}
  - components.tvmtool
{% endif %}
  - .cleaner
{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
  - .mdb-metrics
{% endif %}

mdb-deploy-api-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-deploy-api: '1.9324444'
            - zk-flock
        - require:
            - cmd: repositories-ready

mdb-deploy-api-user:
  user.present:
    - fullname: MDB Deploy API system user
    - name: mdb-deploy-api
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/etc/yandex/mdb-deploy-api/mdb-deploy-api.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-deploy-api.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True

/etc/yandex/mdb-deploy-api/dbpg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dbpg.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True

/lib/systemd/system/mdb-deploy-api.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: mdb-deploy-api-pkgs
        - source: salt://{{ slspath }}/conf/mdb-deploy-api.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

{% if salt['pillar.get']('data:disable_deploy_api', False) %}
mdb-deploy-api-service:
    service.dead:
        - name: mdb-deploy-api
        - enable: False
{% else %}
mdb-deploy-api-service:
    service.running:
        - name: mdb-deploy-api
        - enable: True
        - watch:
            - pkg: mdb-deploy-api-pkgs
            - file: /lib/systemd/system/mdb-deploy-api.service
            - file: /etc/yandex/mdb-deploy-api/*
            - file: /etc/yandex/mdb-deploy/job_result_blacklist.yaml
{% endif %}

{% if salt.pillar.get('data:pg_ssl_balancer') -%}
    {% set key_path = 'cert.key' %}
    {% set crt_path = 'cert.crt' %}
{% else %}
    {% set key_path = 'data:mdb-deploy-api:tls_key' %}
    {% set crt_path = 'data:mdb-deploy-api:tls_crt' %}
{%- endif %}

/etc/nginx/ssl/mdb-deploy-api.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: {{ key_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb-deploy-api.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: {{ crt_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

mdb-developers-present-in-pillar:
    test.check_pillar:
        - listing:
            - 'data:mdb-developers'

/etc/nginx/conf.d/mdb-deploy-api.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - test: mdb-developers-present-in-pillar
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service
