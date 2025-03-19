{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set mongodb_version = salt.mdb_mongodb.version() %}

{% if mongodb.cluster_auth == 'keyfile' %}
{{mongodb.config_prefix}}/mongodb.key:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/templates/mongodb.key
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 400
        - require:
            - file: {{mongodb.config_prefix}}
        - require_in:
{% for srv in mongodb.services_deployed %}
            - service: {{srv}}-service
{% endfor %}
{% endif %}

/home/monitor/.mongopass:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/mongopass
        - context:
            users: [ 'monitor' ]
            mongodb: {{mongodb | tojson}}
        - user: monitor
        - group: monitor
        - makedirs: True
        - mode: 400

/root/.mongopass:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/mongopass
        - context:
            users: [ 'admin', 'monitor' ]
            mongodb: {{mongodb | tojson}}
        - user: root
        - group: root
        - makedirs: True
        - mode: 400

{% set host = salt.grains.get('id') %}
{% set admin_password = mongodb.users['admin'].password %}
{% for srv in mongodb.services_deployed %}
{% set port = mongodb.config.get(srv).net.port %}
/root/.mongouri-admin-{{srv}}:
    file.managed:
        - contents: |
            mongodb://admin:{{admin_password}}@{{host}}:{{port}}/?appname=mongo_admin_shell
        - user: root
        - group: root
        - makedirs: True
        - mode: 400
{% endfor %}

/etc/logrotate.d/mongodb:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mongodb.logrotate
        - mode: 644
        - context:
            mongodb: {{mongodb | tojson}}

{% for srv in mongodb.services_deployed %}
# Pre-create logfiles on cluster init: otherwise pushclient cannot read them
{{ mongodb.config.get(srv).systemLog.path }}:
    file.managed:
        - replace: False
        - mode: 644
        - user: {{ mongodb.user }}
        - group: {{ mongodb.group }}
{% endfor %}

mongodb-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # Some useful things for mongodb
{% for srv in mongodb.services_deployed %}
{# It won't work in case of multiple services on one host but it shouldn't happen #}
{% set port = mongodb.config.get(srv).net.port %}
            alias mongo='/usr/bin/mongo --ipv6 {{mongodb.config.get(srv).cli.ssl_cli_args}} $(cat /root/.mongouri-admin-{{srv}})'
            alias mongo-{{srv}}='/usr/bin/mongo --ipv6 {{mongodb.config.get(srv).cli.ssl_cli_args}} $(cat /root/.mongouri-admin-{{srv}})'
{% endfor %}
        - require_in:
            - file: /root/.bashrc

/root/.mongorc.js:
    file.managed:
        - source: salt://{{ slspath }}/conf/mongorc.js
        - user: root
        - group: root
        - mode: 640

mongodb-packages:
    pkg.installed:
        - order: 1
        - pkgs:
            - mongodb{{ mongodb_version.pkg_suffix }}-server: {{ mongodb_version.pkg_version }}
            - mongodb{{ mongodb_version.pkg_suffix }}-mongos: {{ mongodb_version.pkg_version }}
            - mongodb{{ mongodb_version.pkg_suffix }}-shell: {{ mongodb_version.pkg_version }}
            - mongodb-database-tools: 100.4.1
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
{% if mongodb_version.edition == 'enterprise' %}
            - snmp
            - libsasl2-modules-gssapi-mit
{% endif %}
            - numactl: '2.0.11-2.1'
            - python-pymongo: '3.6.1+dfsg1-1'
            - python3-pymongo
            - python-bson: '3.6.1+dfsg1-1'


{# TODO: Not sure it is really needed, need to check and delete this state #}
shutdown-mongod-org:
    cmd.run:
        - name: 'mongo --port 27017 --eval "db.shutdownServer()" admin'
        - require:
            - pkg: mongodb-packages
        - unless:
            - "stat /etc/init/mongod.conf.disabled || ! stat /etc/init/mongod.conf"

disable-mongod-org:
    cmd.run:
        - name: 'mv /etc/init/mongod.conf /etc/init/mongod.conf.disabled'
        - unless:
            - "stat /etc/init/mongod.conf.disabled || ! stat /etc/init/mongod.conf"

/var/log/mongodb/mongod.log:
    file.absent

/usr/local/yandex/mongodb_wait_started.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mongodb_wait_started.py
        - mode: 755
        - makedirs: True

{% for srv in mongodb.services_deployed %}
/etc/yandex/mongoctl-{{ srv }}.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/mongoctl.conf
        - mode: 644
        - template: jinja
        - makedirs: True
        - context:
            srv: {{ srv | tojson }}
            mongodb: {{ mongodb | tojson }}
{% endfor %}

/usr/local/yandex/mongoctl.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mongoctl.py
        - mode: 755
        - makedirs: True
        - require:
            - file: /root/.mongopass
{% for srv in mongodb.services_deployed %}
            - file: /etc/yandex/mongoctl-{{srv}}.conf
{% endfor %}

/usr/local/yandex/mdb-disk-watcher.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb_disk_watcher.py
        - mode: 755
        - makedirs: True
        - require:
            - file: /root/.mongopass

/etc/cron.d/mdb-mongodb-disk-watcher:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set config = mongodb.config.get(srv) %}
             * * * * * root /usr/local/yandex/mdb-disk-watcher.py --pid-file {{config.processManagement.pidFilePath}} --flag-file-locked /tmp/mdb-mongo-fsync.{{srv}}.locked --flag-file-unlocked /tmp/mdb-mongo-fsync.{{srv}}.unlocked --free-mb-limit {{salt.mdb_mongodb_helpers.get_mongo_ro_limit(salt.pillar.get('data:dbaas:space_limit'))}} >/dev/null 2>&1
{% endfor %}
        - mode: 644
        - require:
            - file: /usr/local/yandex/mdb-disk-watcher.py

{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%     set config = mongodb.config.get(srv) %}
{%     set uri =  'mongodb://monitor:{0}@{1}:{2}/admin?appname=mdb_sessions_count_wd{3}'.format(mongodb.users['monitor'].password, salt.grains.get('id'), config.net.port, config.cli.ssl_uri_args) %}
/usr/local/yandex/mongodb_sessions_count_wd.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/templates/mongodb_sessions_count_wd.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - context:
            uri: {{ uri }}

/etc/cron.d/mongodb_sessions_count_wd:
    file.managed:
        - contents: |
            PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
            */10 * * * * root sleep $((RANDOM \% 300)); flock -n /tmp/mongodb_sessions_count_wd.lock timeout 1200 /usr/local/yandex/mongodb_sessions_count_wd.py >/dev/null 2>&1
        - mode: 644
        - require:
            - file: /usr/local/yandex/mongodb_sessions_count_wd.py
{% endfor %}

/usr/local/yandex/mdb-load-monitor.py:
    file.absent

{% for srv in mongodb.services_deployed %}
/etc/yandex/mdb-load-monitor-{{srv}}.conf:
    file.absent
{% endfor %}

/etc/cron.d/mdb-load-monitor:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
{% for srv in mongodb.services_deployed %}
{%   set config = mongodb.config.get(srv) %}
             # Every Monday drop /tmp/admin-marked-as-failed file
             1 11 * * 1 root /bin/rm -f /tmp/admin-marked-as-broken
{% endfor %}
        - mode: 644

{% if salt.grains.get('virtual') == 'lxc'%}
/etc/cron.d/mongo_porto_chown:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             @reboot root chown mongodb.mongodb /var/lib/mongodb >/dev/null 2>&1
        - mode: 644
{% endif %}

{# To properly start mongo* we need to have valid /root/.mongopass file #}
include:
{% for srv in mongodb.services_deployed %}
    - .{{ srv }}-service
{% endfor %}

extend:
{% for srv in mongodb.services_deployed %}
    {{ srv }}-service:
        service.running:
            - require:
                - file: /root/.mongopass
                - file: /root/.mongouri-admin-{{srv}}
                - file: mongodb-root-bashrc
                - file: /root/.mongorc.js
{% endfor %}
