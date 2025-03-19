old-openjdk:
    pkg.purged:
        - pkgs:
            - openjdk-11-jre-headless
        - require_in:
            - pkg: openjdk

openjdk:
    pkg.installed:
        - pkgs:
            - openjdk-17-jre-headless

include:
  - components.web-api-base
  - components.blackbox-sso

/etc/nginx/ssl/gerrit.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/gerrit.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/gerrit.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-gerrit.conf' }}
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

mdb-gerrit-group:
    group.present:
        - name: gerrit
        - system: True

mdb-gerrit-user:
    user.present:
        - name: gerrit
        - gid_from_name: True
        - home: /var/lib/gerrit
        - empty_password: True
        - shell: /usr/sbin/nologin
        - system: True
        - require:
            - group: mdb-gerrit-group

/var/lib/gerrit/.ssh/config:
    file.managed:
        - source: salt://{{ slspath + '/conf/ssh-config' }}
        - require:
            - user: mdb-gerrit-user

/etc/supervisor/conf.d/gerrit.conf:
    file.managed:
        - source: salt://{{ slspath + '/conf/supervisor-gerrit.conf' }}
        - require:
            - user: mdb-gerrit-user
        - watch_in:
            - supervisord: gerrit-supervised

gerrit-supervised:
    supervisord.running:
        - name: gerrit
        - update: True
        - require:
            - file: /var/lib/gerrit/.ssh/config

gerrit-backup-pkgs:
    pkg.installed:
        - pkgs:
            - duplicity

/etc/cron.d/gerrit-backup:
    file.managed:
        - source: salt://{{ slspath + '/conf/gerrit-backup.cron' }}
        - template: jinja
        - require:
            - pkg: gerrit-backup-pkgs

/etc/logrotate.d/gerrit-backup:
    file.managed:
        - source: salt://{{ slspath + '/conf/gerrit-backup.logrotate' }}
        - require:
            - pkg: gerrit-backup-pkgs
