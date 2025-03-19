# nginx should be already installed

include:
  - components.monrun2.mdb_cms
  - components.tvmtool
  - .mdb-metrics

mdb-cms-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-cms: '2.9741997'
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

/opt/yandex/mdb-cms/etc/mdb-cms.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-cms.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-cms-pkgs

/opt/yandex/mdb-cms/etc/dbpg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dbpg.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-cms-pkgs


/opt/yandex/mdb-cms/etc/metadb.yaml:
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

mdb-cms-service:
    service.running:
        - name: mdb-cms
        - enable: True
        - require:
            - pkg: mdb-cms-pkgs
            - user: mdb-cms
        - watch:
            - pkg: mdb-cms-pkgs
            - file: /opt/yandex/mdb-cms/etc/mdb-cms.yaml
            - file: /opt/yandex/mdb-cms/etc/dbpg.yaml
            - file: /etc/systemd/system/mdb-cms.service

/etc/systemd/system/mdb-cms.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/systemd.conf' }}
        - require:
            - pkg: mdb-cms-pkgs


/etc/nginx/conf.d/mdb-cms.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - pkg: mdb-cms-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb-cms.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - require:
            - /etc/nginx/ssl
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/mdb-cms.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - require:
            - /etc/nginx/ssl
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/cron.d/cms-autoduty:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/cron/autoduty.cron
        - mode: 644
        - require:
            - pkg: mdb-cms-pkgs
