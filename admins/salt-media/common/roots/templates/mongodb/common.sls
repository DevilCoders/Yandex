{% from slspath + '/map.jinja' import mongodb,cluster with context %}

include:
  - .monrun-common
  - .daytime
{% if mongodb.stock and "stock-tgz" not in mongodb %}
  - .repo
{%- endif %}

{%- if mongodb.keyFile and not mongodb.keyFile_nomanage %}
{{mongodb.keyFile}}:
  file.managed:
    {% if mongodb.get("keyFilePillar", False) %}
    - contents: {{ salt['pillar.get']('mongodb:keyFilePillar') | json }}
    {% else %}
    - source:
      - salt://files/{{cluster}}{{mongodb.keyFile}}
      - salt://{{cluster}}{{mongodb.keyFile}}
    {% endif %}
    - mode: 0400
    - user: mongodb
    - group: mongodb
{%- endif %}

{%- if mongodb["logrotate-files"] %}
/etc/logrotate.d/mongodb.conf:
  file.absent
/etc/logrotate.d/mongodbcfg.conf:
  file.absent
/etc/logrotate.d/mongos.conf:
  file.absent
{% for file in mongodb["logrotate-files"] %}
{{file}}:
  file.managed:
    - source:
      - salt://files/{{cluster}}{{file}}
      - salt://{{cluster}}{{file}}
      - salt://templates/mongodb/files{{file}}
    - follow_symlinks: False
    - replace: True
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
{% endfor %}
{%- endif %}

{% for file in mongodb["common-files"] %}
{{file}}:
  file.managed:
    - source:
      - salt://files/{{cluster}}{{file}}
      - salt://{{cluster}}{{file}}
      - salt://templates/mongodb/files{{file}}
    - follow_symlinks: False
    - replace: True
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
{% endfor %}
{% for file in mongodb["exec-files"] %}
{{file}}:
  file.managed:
    - source: salt://templates/mongodb/files{{ file }}
    - follow_symlinks: False
    - replace: True
    - mode: 755
    - user: root
    - group: root
    - makedirs: true
{% endfor %}

{% if mongodb.mongod %}
{%- if grains['oscodename'] == "trusty" %}
pymonmongo:
  pkg.installed
{%- endif %}

/etc/mongodb.conf:
  file.managed:
    - follow_symlinks: False
    - replace: True
    - source:
      - salt://files/{{cluster}}/etc/mongodb.conf
      - salt://{{cluster}}/etc/mongodb.conf
      - salt://templates/mongodb/files/etc/mongodb-vanilla.conf
    - template: jinja
    - context:
      mongodb: {{ mongodb }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/monitoring/mongodb_indexes/conf.d:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
{% endif %}

{% if not mongodb.stock %}
mongodb:
  pkg:
    - installed
    {%- if mongodb.version|default(False) and not mongodb.stock %}
    - version: {{ mongodb.version }}
    {%- endif %}

{% else %} {# if stock #}
  {% if "stock-tgz" not in mongodb %}
mongodb:
  pkg:
    - installed
    - pkgs:
      {%- if mongodb.version|default(False) %}
      - mongodb-org: {{ mongodb.version }}
      - mongodb-org-server: {{ mongodb.version }}
      - mongodb-org-shell: {{ mongodb.version }}
      - mongodb-org-mongos: {{ mongodb.version }}
      - mongodb-org-tools: {{ mongodb.version }}
      {%- else %}
      - mongodb-org
      {%- endif %}
      - numactl
  {% else %}
/opt/mongo.tgz:
  file.managed:
    - source: {{mongodb["stock-tgz"]}}  # for example http url to mongodb-linux-x86_64-ubuntu1404-3.6.11.tgz
    - source_hash: {{mongodb["stock-tgz-hash"]}}
  cmd.run:
    - name: tar --strip-components=2 -vxzf /opt/mongo.tgz -C /usr/bin/
    - onchanges:
      - file: /opt/mongo.tgz
  {% endif %}
{% endif %} # END: if mongodb.stock


/var/spool/monrun:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

python-pymongo:
  pkg:
    - installed
python-lockfile:
  pkg:
    - installed

{% if mongodb.mongod %}
/var/run/mongodb:
  file.directory:
    - mode: 755
    - user: mongodb
    - group: mongodb

mongod:
  service.running:
    - name: mongodb
    - require:
      - file: /etc/mongodb.conf
  {%- if not mongodb.stock %}
      - pkg: mongodb
  {%- else %}
    {% if "stock-tgz" not in mongodb %}
      - pkg: mongodb
    {% endif %}
      - file: /etc/default/mongodb
/etc/default/mongodb:
  file.managed:
    - template: jinja
    - source: salt://templates/mongodb/files/etc/default/mongodb
    - context:
        m: {{mongodb}}
  {%- endif %}
{% endif %}

/var/log/mongodb:
  file.directory:
    - mode: 755
    - user: mongodb
    - group: mongodb
    - makedirs: True

{% if mongodb["managed-database"] %}
mysqladmin:
  user:
    - present
    - system: True
    - shell: '/bin/bash'
    - createhome: True
    - home: '/var/mysqladmin'
    - groups:
      - nogroup

/var/mysqladmin/.ssh:
  file.directory:
    - user: mysqladmin
    - group: nogroup
    - mode: 700
    - makedirs: True

/var/lib/dbmanager:
  file.directory:
    - user: mysqladmin
    - group: nogroup
    - mode: 700
    - makedirs: True

/var/mysqladmin/.ssh/authorized_keys:
  file.managed:
    - source: salt://templates/mongodb/files/var/mysqladmin/.ssh/authorized_keys
    - mode: 600
    - user: mysqladmin
    - group: nogroup
    - makedirs: True

/etc/sudoers.d/mysqladmin-mongo:
  file.managed:
    - contents: |
        mysqladmin ALL=(mongodb) NOPASSWD: ALL
        mysqladmin ALL=(ALL) NOPASSWD: /usr/sbin/service mongodb *
    - mode: 440
    - user: root
    - file: root

/var/mysqladmin/js:
  file.recurse:
    - source: salt://templates/mongodb/files/var/mysqladmin/js
    - file_mode: 644
    - user: mysqladmin
    - group: nogroup
    - exclude_pat: .svn*

/etc/rsyncd.secrets:
  {%- if mongodb.rsyncd_secrets %}
  file.managed:
    - contents: {{ mongodb.rsyncd_secrets | json }}
  {%- else %}
  yafile.managed:
    - source:
      - salt://secrets/mongodb/etc/rsyncd.secrets
      - salt://secrets/{{cluster}}/etc/rsyncd.secrets
      - salt://files/{{cluster}}/etc/rsyncd.secrets
  {%- endif %}
    - mode: 600
    - user: root
    - group: root
    - makedirs: true

/etc/rsyncd.conf:
  file.managed:
    - source: salt://files/{{cluster}}/etc/rsyncd.conf
    - mode: 644
    - user: root
    - group: root
    - makedirs: true

/etc/default/rsync:
  file.replace:
    - pattern: 'RSYNC_ENABLE=false'
    - repl: 'RSYNC_ENABLE=true'

rsync:
  pkg:
    - installed
  service.running:
    - require:
      - file: /etc/default/rsync
      {%- if mongodb.rsyncd_secrets %}
      - file: /etc/rsyncd.secrets
      {%- else %}
      - yafile: /etc/rsyncd.secrets
      {%- endif %}
      - file: /etc/rsyncd.conf
{% endif %}

{% if mongodb.get("mongorcPillar", False) %}
/root/.mongorc.js:
  file.managed:
    - contents: {{ salt['pillar.get']('mongodb:mongorcPillar') | json }}
    - mode: 640
    - user: root
    - group: root
{% endif %}

{% if mongodb.mongo_hack %}
/etc/mongorc.js:
  file.managed:
    - source:
      - salt://{{ slspath }}/files/etc/mongorc.js
      - salt://{{ slspath }}/etc/mongorc.js
    - mode: 644
    - user: root
    - group: root
    - require:
      - file: /etc/mongorc.d/mongo-hack.js

/etc/mongorc.d/mongo-hack.js:
  file.managed:
    - source:
      - salt://{{ slspath }}/files/etc/mongorc.d/mongo-hack.js
      - salt://{{ slspath }}/etc/mongorc.d/mongo-hack.js
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endif %}

{# CleanUP #}
/usr/sbin/mongodb-logrotate:
  file.absent
/etc/cron.d/mongodb-logrotate:
  file.absent

{% if mongodb.stock %}
mongod-org-stock-daemon:
  service.dead:
    - name: mongod
    - enable: False
    - onlyif: test -s /etc/init/mongod.conf
    - order: 0
  file.absent:
    - names:
      - /etc/init/mongod.conf
      {%- if grains['oscodename'] == "xenial" %}
      - /etc/init/mongodb.conf {# use systemd service here #}
      {%- endif %}
      {%- if "stock-tgz" in mongodb %}
      - /etc/apt/sources.list.d/mongodb.org.list
      {%- endif %}
    - order: 1
    - require:
      - service: mongod-org-stock-daemon
/etc/mongod.conf:
  file.absent:
    - order: last
{% endif %}
