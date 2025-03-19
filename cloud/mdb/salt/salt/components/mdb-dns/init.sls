# Supervisor and nginx should be already installed
# Not adding it because of conflicts on initial deploy

include:
    - components.monrun2.mdb-dns
    - .mdb-metrics

mdb-dns-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-dns: '1.9268650'

mdb-dns-group:
  group.present:
    - name: mdb-dns
    - system: True

mdb-dns-user:
  user.present:
    - fullname: MDB DNS system user
    - name: mdb-dns
    - gid_from_name: True
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data
        - mdb-dns
    - require:
        - group: mdb-dns-group

/opt/yandex/mdb-dns/etc/mdb-dns.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-dns.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-dns-pkgs

/opt/yandex/mdb-dns/etc/metadbpg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/metadbpg.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-dns-pkgs

/opt/yandex/mdb-dns/etc/mdbhsspg.yaml:
    file.absent

mdb-dns-supervised:
    supervisord.running:
        - name: mdb-dns
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-dns
        - watch:
            - pkg: mdb-dns-pkgs
            - file: /etc/supervisor/conf.d/mdb-dns.conf
            - file: /opt/yandex/mdb-dns/etc/mdb-dns.yaml
            - file: /opt/yandex/mdb-dns/etc/mdbhsspg.yaml

/etc/supervisor/conf.d/mdb-dns.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: mdb-dns-pkgs

{% if salt.pillar.get('data:pg_ssl_balancer') -%}
    {% set key_path = 'cert.key' %}
    {% set crt_path = 'cert.crt' %}
{% else %}
    {% set key_path = 'data:mdb-dns:tls_key' %}
    {% set crt_path = 'data:mdb-dns:tls_crt' %}
{%- endif %}

/etc/nginx/ssl/mdb-dns.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: {{ key_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb-dns.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: {{ crt_path }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/mdb-dns.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - pkg: mdb-dns-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/usr/local/yasmagent/CONF/agent.mdbdns.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.mdbdns.conf
        - mode: 644
        - user: monitor
        - group: monitor
        - require:
            - pkg: yasmagent
        - watch_in:
            - service: yasmagent

