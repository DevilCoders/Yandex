{% set zk = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

include:
    - .service
    - .cleanup

zookeeper-packages:
    pkg.installed:
        - pkgs:
{% for pkg, ver in zk.packages.items() %}
            - {{ pkg }}: {{ ver }}
{% endfor %}
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
            - python-click: 6.7-3
        - ignore_epoch: True

/usr/local/bin/zkCli.sh:
    file.managed:
        - source: salt://{{ slspath }}/zkCli.sh
        - mode: 750
        - require:
            - pkg: zookeeper-packages

/usr/local/yandex/zk_wait_started.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk_wait_started.py
        - mode: 755
        - makedirs: True
        - require:
            - pkg: zookeeper-packages
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}:
    file.directory:
        - user: zookeeper
        - group: zookeeper
        - mode: 755
        - makedirs: True
        - require:
            - pkg: zookeeper-packages
            - user: zookeeper-user
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}/log4j.properties:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/log4j.properties
        - mode: 644
        - makedirs: True
        - require:
            - file: {{ zk.config_prefix }}
        - require_in:
            - test: zookeeper-service-req
            - alternatives: zookeeper-register-conf
        - context:
            zk: {{ zk | tojson }}

{% if salt.pillar.get('data:unmanaged:enable_zk_tls', False) %}
{{ zk.config_prefix }}/ssl:
    file.directory:
        - user: zookeeper
        - group: zookeeper
        - makedirs: True
        - mode: 755

{{ zk.config_prefix }}/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: root
        - group: zookeeper
        - mode: 640
        - require:
            - file: {{ zk.config_prefix }}/ssl
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: root
        - group: zookeeper
        - mode: 640
        - require:
            - file: {{ zk.config_prefix }}/ssl
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: root
        - group: zookeeper
        - mode: 640
        - require:
            - file: {{ zk.config_prefix }}/ssl
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}/ssl/server.jks:
    mdb_zookeeper.ensure_keystore:
        - path: {{ zk.config_prefix }}/ssl
        - host: {{ salt.grains.get('fqdn') }}
        - user: root
        - group: zookeeper
        - mode: 640
        - config: {{ zk.config_prefix }}/zoo.cfg
        - require:
            - mdb_zookeeper: {{ zk.config_prefix }}/zoo.cfg
            - file: {{ zk.config_prefix }}/ssl/server.crt
        - require_in:
            - test: zookeeper-service-req

{{ zk.config_prefix }}/ssl/truststore.jks:
    mdb_zookeeper.ensure_truststore:
        - path: {{ zk.config_prefix }}/ssl
        - host: {{ salt.grains.get('fqdn') }}
        - user: root
        - group: zookeeper
        - mode: 640
        - config: {{ zk.config_prefix }}/zoo.cfg
        - require:
            - mdb_zookeeper: {{ zk.config_prefix }}/zoo.cfg
            - file: {{ zk.config_prefix }}/ssl/allCAs.pem
        - require_in:
            - test: zookeeper-service-req
{% endif %}

{{ zk.config_prefix }}/java.security:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/java.security
        - mode: 644
        - makedirs: True
        - require:
            - file: {{ zk.config_prefix }}
        - require_in:
            - test: zookeeper-service-req

{% if salt.pillar.get('data:zk:scram_auth_enabled', False) %}
{{ zk.config_prefix }}/zookeeper_jaas.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/zookeeper_jaas.conf
        - mode: 644
        - makedirs: True
        - require:
            - file: {{ zk.config_prefix }}
        - require_in:
            - test: zookeeper-service-req
{% endif %}

/etc/logrotate.d/zookeeper.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - require:
            - pkg: zookeeper-packages

zookeeper-register-conf:
    alternatives.install:
        - name: zookeeper-conf
        - link: '/etc/zookeeper/conf'
        - path: {{ zk.config_prefix }}
        - priority: 50
        - require:
            - file: {{ zk.config_prefix }}

/etc/zookeeper/conf:
    file.symlink:
        - target: /etc/alternatives/zookeeper-conf
        - require:
            - alternatives: zookeeper-register-conf
        - require_in:
            - test: zookeeper-service-req

/var/log/zookeeper:
    file.directory:
        - user: zookeeper
        - group: zookeeper
        - mode: 755
        - require:
            - user: zookeeper-user
        - require_in:
            - test: zookeeper-service-req

{{ zk.config.dataDir }}:
    file.directory:
        - user: zookeeper
        - group: zookeeper
        - dir_mode: 755
        - file_mode: 644
        - makedirs: True
        - recurse:
            - user
            - group
            - mode
        - require:
            - pkg: zookeeper-packages
            - user: zookeeper-user

{{ zk.config.dataDir }}/myid:
    file.managed:
        - user: zookeeper
        - group: zookeeper
        - mode: 644
        - follow_symlinks: False
        - template: jinja
        - source: salt://{{slspath}}/conf/myid
        - require:
            - file: {{ zk.config.dataDir }}
        - require_in:
            - test: zookeeper-service-req
        - context:
            zk: {{ zk | tojson }}

# Workaround for ZOOKEEPER-3056
{{ zk.config.dataDir }}/version-2/snapshot.0:
    file.managed:
        - source: salt://{{slspath}}/conf/snapshot.0
        - makedirs: True
        - user: zookeeper
        - group: zookeeper
        - mode: 644
        - require:
            - user: zookeeper-user
        - require_in:
            - service: zookeeper-service
        - unless: ls {{ zk.config.dataDir }}/version-2/snapshot.*

zookeeper-group:
    group.present:
        - name: zookeeper
        - require:
            - pkg: zookeeper-packages

zookeeper-user:
    user.present:
        - fullname: Zookeeper service user
        - name: zookeeper
        - gid: zookeeper
        - empty_password: False
        - shell: /bin/false
        - system: True
        - require:
            - group: zookeeper-group

/etc/cron.d/zk-rotate-xact-logs:
    file.managed:
        - mode: 644
        - follow_symlinks: False
        - template: jinja
        - context:
            zk: {{ zk | tojson }}
        - source: salt://{{slspath}}/conf/zk-rotate-xact-logs
        - require:
            - file: /usr/local/yandex/log_curator.py

/var/log/yandex:
    file.directory:
        - user: zookeeper
        - group: zookeeper
        - mode: 755
        - makedirs: True
        - require:
            - user: zookeeper-user
        - require_in:
            - file: /usr/local/yandex/zk_cleanup.py
            - file: /usr/local/yandex/zk_cluster_reconfig.py

/usr/local/yandex/zk_cluster_reconfig.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk_cluster_reconfig.py
        - mode: 755
        - makedirs: True
        - require:
            - pkg: zookeeper-packages
        - require_in:
            - test: zookeeper-service-req

/usr/local/yandex/ensure_not_leader.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_not_leader.py
        - user: root
        - group: root
        - mode: 755
        - makedirs: True
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/ensure_no_primary.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_no_primary.sh
        - user: root
        - group: root
        - mode: 755
        - makedirs: True
        - require:
            - file: /usr/local/yandex
            - file: /usr/local/yandex/ensure_not_leader.py

