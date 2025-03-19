# Supervisor and nginx should be already installed
# Not adding it because of conflicts on initial deploy
include:
    - components.monrun2.mdb-health
    - .mdb-metrics

{% if salt.pillar.get('data:mdb_health:disable-health', False) %}

mdb-health-pkgs:
    pkg.purged:
        - name: mdb-health

mdb-health-group:
  group.absent:
    - name: mdb-health
    - require:
        - user: mdb-health-user

mdb-health-user:
  user.absent:
    - name: mdb-health

/opt/yandex/mdb-health/etc/mdbh.yaml:
  file.absent:
    - pkg: mdb-health-pkgs

/opt/yandex/mdb-health/etc/mdbhdsredis.yaml:
  file.absent:
    - pkg: mdb-health-pkgs

/opt/yandex/mdb-health/etc/mdbhsspg.yaml:
  file.absent:
    - pkg: mdb-health-pkgs

mdb-health-supervised:
    supervisord.dead:
        - name: mdb-health

/etc/supervisor/conf.d/mdb-health.conf:
    file.absent:
        - require:
            - supervisord: mdb-health

{% else %}

mdb-health-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-health: '1.9630131'

mdb-health-group:
  group.present:
    - name: mdb-health
    - system: True

mdb-health-user:
  user.present:
    - fullname: MDB Health system user
    - name: mdb-health
    - gid_from_name: True
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data
    - require:
        - group: mdb-health-group

/opt/yandex/mdb-health/etc/mdbh.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdbh.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-health-pkgs

/opt/yandex/mdb-health/etc/mdbhdsredis.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdbhdsredis.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-health-pkgs

/opt/yandex/mdb-health/etc/mdbhsspg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdbhsspg.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-health-pkgs

mdb-health-supervised:
    supervisord.running:
        - name: mdb-health
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-health
        - watch:
            - pkg: mdb-health-pkgs
            - file: /etc/supervisor/conf.d/mdb-health.conf
            - file: /opt/yandex/mdb-health/etc/mdbh.yaml
            - file: /opt/yandex/mdb-health/etc/mdbhdsredis.yaml
            - file: /opt/yandex/mdb-health/etc/mdbhsspg.yaml

/etc/supervisor/conf.d/mdb-health.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: mdb-health-pkgs

{% endif %}

{% if salt.pillar.get('data:pg_ssl_balancer') -%}
    {% set key_path = 'cert.key' %}
    {% set crt_path = 'cert.crt' %}
{% else %}
    {% set key_path = 'data:mdb-health:tls_key' %}
    {% set crt_path = 'data:mdb-health:tls_crt' %}
{%- endif %}

/etc/nginx/ssl/mdb-health.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: {{ key_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb-health.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: {{ crt_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/mdb-health.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - pkg: mdb-health-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/usr/local/yasmagent/mdbhealth_getter.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
        - require:
              - file: /usr/local/yasmagent/redis_getter.py
              - file: /usr/local/yasmagent/default_getter.py
{% if salt['pillar.get']('data:use_yasmagent', True) %}
        - require_in:
              - file: mdbhealth-yasmagent-instance-getter-config

/usr/local/yasmagent/CONF/agent.mdbhealth.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.mdbhealth.conf
        - mode: 644
        - user: monitor
        - group: monitor
        - require:
            - pkg: yasmagent
        - watch_in:
            - service: yasmagent
{% endif %}
