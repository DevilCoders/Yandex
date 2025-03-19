# avoid SLS conflict between old and new api components in dataproc-infra-tests
{% if not salt['pillar.get']('data:is_infratest_env', False) %}
include:
    - components.logrotate
    - .mdb-metrics
{% if salt['pillar.get']('data:use_pushclient', False) %}
    - .push-client
{% endif %}
{% if salt['pillar.get']('data:mdb-search-reindexer') %}
    - .search-reindexer
{% endif %}
{% endif %}

mdb-internal-api-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-internal-api: '1.9770733'
        - require:
            - cmd: repositories-ready

mdb-internal-api-user:
  user.present:
    - fullname: MDB Internal API system user
    - name: mdb-internal-api
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/lib/systemd/system/mdb-internal-api.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-internal-api.service
        - mode: 644
        - require:
            - pkg: mdb-internal-api-pkgs # Guarantee that if service was in old package and remove in new one, it wont remove THIS service
            - user: mdb-internal-api-user
        - onchanges_in:
            - module: systemd-reload

/etc/yandex/mdb-internal-api:
    file.directory:
        - user: mdb-internal-api
        - group: www-data
        - mode: '0750'
        - makedirs: True
        - require:
            - user: mdb-internal-api-user

/etc/yandex/mdb-internal-api/mdb-internal-api.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-internal-api.yaml' }}
        - mode: '0640'
        - user: mdb-internal-api
        - group: www-data
        - makedirs: True
        - require:
            - user: mdb-internal-api-user
            - file: /etc/yandex/mdb-internal-api

/etc/yandex/mdb-internal-api/console_default_resources.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/console_default_resources.yaml' }}
        - mode: '0640'
        - user: mdb-internal-api
        - group: www-data
        - makedirs: True
        - require:
            - user: mdb-internal-api-user
            - file: /etc/yandex/mdb-internal-api

/var/log/mdb-internal-api:
    file.directory:
        - user: mdb-internal-api
        - group: www-data
        - mode: 755
        - recurse:
            - user
            - group
            - mode
        - makedirs: True
        - require:
            - user: mdb-internal-api-user

/etc/logrotate.d/mdb-internal-api:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

mdb-internal-api-service:
    service.running:
        - name: mdb-internal-api
        - enable: True
        - watch:
            - pkg: mdb-internal-api-pkgs
            - file: /lib/systemd/system/mdb-internal-api.service
            - file: /etc/yandex/mdb-internal-api/mdb-internal-api.yaml
            - file: /etc/yandex/mdb-internal-api/console_default_resources.yaml
            - file: /var/log/mdb-internal-api
