include:
  - components.monrun2.mdb-cms-grpcapi
  - .mdb-metrics

mdb-cms-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-cms-instance-api: '1.9765850'
        - require:
            - cmd: repositories-ready

mdb-cms-user:
  user.present:
    - fullname: MDB CMS API system user
    - name: mdb-cms
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/etc/yandex/mdb-cms/mdb-cms.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-cms.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-cms-pkgs

/etc/yandex/mdb-cms/dbpg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dbpg.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-cms-pkgs


/etc/yandex/mdb-cms/metadb.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/metadb.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-cms-pkgs

/etc/logrotate.d/mdb-cms:
    file.managed:
       - source: salt://{{ slspath }}/conf/logrotate.conf
       - template: jinja
       - mode: 644
       - user: root
       - group: root

/var/log/mdb-cms:
    file.directory:
        - user: mdb-cms
        - dir_mode: 755
        - file_mode: 644
        - makedirs: true
        - require:
            - pkg: mdb-cms-pkgs
            - user: mdb-cms

mdb-cms-instance-api-service:
    service.running:
        - name: mdb-cms-instance-api
        - enable: True
        - require:
            - pkg: mdb-cms-pkgs
            - user: mdb-cms
        - watch:
            - pkg: mdb-cms-pkgs
            - file: /etc/yandex/mdb-cms/mdb-cms.yaml
            - file: /etc/yandex/mdb-cms/dbpg.yaml
            - file: /etc/yandex/mdb-cms/metadb.yaml
            - file: /etc/systemd/system/mdb-cms-instance-api.service

/etc/systemd/system/mdb-cms-instance-api.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-cms-instance-api.service' }}
        - require:
            - pkg: mdb-cms-pkgs
