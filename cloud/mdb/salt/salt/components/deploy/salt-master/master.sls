{% set salt_version = salt['pillar.get']('data:salt_version', '3000.9+ds-1+yandex0') %}
{% set osrelease = salt['grains.get']('osrelease') %}
{% if osrelease == '18.04' %}
    {% set kazoo_version = '2.5.0-2yandex' %}
{% else %}
    {% set kazoo_version = '2.2.1-yandex1' %}
{% endif %}

salt-master:
    service.running:
        - restart: True
        - require:
            - pkg: salt-packages
            - test: srv-configured
        - watch:
            - pkg: salt-dependency-packages
            - pkg: salt-packages
            - file: /etc/salt/master

salt-packages:
    pkg.installed:
        - pkgs:
            - salt-master: {{ salt_version }}
        - require:
            - pkg: salt-dependency-packages
            - cmd: repositories-ready

salt-dependency-packages:
    pkg.installed:
        - pkgs:
            - python-requests
            - python-openssl
            - python-cherrypy3
            - python3-cherrypy3
            - python-gnupg
            - python-nacl
            - python3-nacl
            - python-paramiko
            - python3-paramiko
            - python-redis
            - python3-redis
            - python-git
            - python-kazoo: {{ kazoo_version }}
            - python3-kazoo
            - zk-flock
            - rsync
            - python-jwt
            - python3-jwt
            - python3-lxml
            - python-setproctitle
{% if salt['pillar.get']('data:salt_master:use_prometheus_metrics') %}
            - python-prometheus-client
            - python3-prometheus-client
{% endif %}
        - require:
            - cmd: repositories-ready

/etc/salt/master:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/master
        - mode: 644
        - user: root
        - group: root
        - watch_in:
            - service: salt-master

/etc/cron.d/salt-chown:
    file.managed:
        - source: salt://{{ slspath }}/conf/salt-chown
        - mode: 644
        - user: root
        - group: root

/root/.ssh/authorized_keys2:
    file.append:
        - source: salt://{{ slspath }}/conf/authorized_keys2
        - template: jinja
        - require:
            - pkg: common-packages

/root/.ssh/id_ecdsa:
    file.managed:
        - source: salt://{{ slspath }}/conf/id_ecdsa
        - template: jinja
        - mode: 600
        - user: root
        - group: root
        - require:
            - pkg: common-packages

/etc/salt/pki/master/mail_salt.pem:
    file.managed:
       - contents_pillar: data:config:salt:master:private
       - user: root
       - group: root

/etc/salt/pki/master/mail_salt.pub:
    file.managed:
       - contents_pillar: data:config:salt:master:public
       - user: root
       - group: root

/etc/salt/ext_pillars:
    file.managed:
        - source: salt://{{ slspath }}/conf/ext_pillars
        - template: jinja
        - makedirs: True
        - user: root
        - group: root
        - watch_in:
            - service: salt-master

/etc/salt/master_modules:
    file.recurse:
        - source: salt://{{ slspath }}/modules
        - makedirs: True
        - include_empty: True
        - clean: True
        - exclude_pat: E@(.*__pycache__$)|(.*\.pyc$)
        - user: root
        - group: root

old-shm-pillars-clean:
    file.tidied:
        - name: /dev/shm
        - matches:
            - '^dbaas.*'
        - require:
            - file: /etc/salt/master_modules

old-shm-dom0-clean:
    file.tidied:
        - name: /dev/shm
        - matches:
            - '^dom0porto.*'
        - require:
            - file: /etc/salt/master_modules

{% if salt['pillar.get']('data:salt_master:use_prometheus_metrics') %}
/etc/salt/master_engines/prometheus_metrics:
    file.recurse:
        - source: salt://{{ slspath }}/engines/prometheus_metrics
        - makedirs: True
        - include_empty: True
        - clean: True
        - exclude_pat: E@(^venv$)|(.*\.pyc$)|(.*__pycache__$)
        - user: root
        - group: root
        - watch_in:
            - service: salt-master
        - require_in:
            - file: /etc/salt/master


{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
# Cause default prometheus exporter can binds only to IPV4 address
maps-127.0.0.1-to-localhost:
    host.present:
        - ip: '127.0.0.1'
        - names:
            - localhost
{% endif %}
{% endif %}

/etc/salt/master.d/yav.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/yav.conf
        - template: jinja
        - makedirs: True
        - user: root
        - group: root
        - watch_in:
            - service: salt-master

salt-image-s3-configured:
    test.check_pillar:
        - present:
              - data:s3:access_key_id
              - data:s3:access_secret_key
              - data:s3:host
        - require_in:
              - file: /root/.s3cfg

/root/.s3cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/s3cfg
        - mode: 600
        - user: root
        - group: root

/etc/yandex/mdb-deploy/update_salt_image.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/update_salt_image.conf
        - mode: 600
        - user: root
        - group: root
        - require:
              - file: /etc/yandex/mdb-deploy

/etc/cron.yandex/update_salt_image.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/update_salt_image.py
        - mode: 755
        - user: root
        - group: root
        - require:
              - file: /root/.s3cfg
              - file: /etc/yandex/mdb-deploy/update_salt_image.conf
              - pkg: update_salt_image-dependency-packages

{% if salt.pillar.get('data:salt_master:use_s3_images', True) %}

/etc/cron.d/update-salt-image:
    file.managed:
        - source: salt://{{ slspath }}/conf/update-salt-image.cron
        - mode: 644

{% endif %}

analyze_versions-dependency-packages:
    pkg.installed:
        - pkgs:
            - python3-humanfriendly
            - python3-xmltodict
        - require:
            - cmd: repositories-ready

update_salt_image-dependency-packages:
    pkg.installed:
        - pkgs:
              - python3
              - s3cmd
        - require:
              - cmd: repositories-ready
