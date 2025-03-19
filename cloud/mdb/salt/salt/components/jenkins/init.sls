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

jenkins:
  pkg.installed:
    - pkgs:
      - jenkins: 2.361+yandex0
    - require:
      - pkg: openjdk

include:
  - components.web-api-base
  - components.logrotate
  - components.monrun2.jenkins
  - components.blackbox-sso

/usr/local/yandex/jenkins_gracefull_restart.py:
    file.managed:
        - source: salt://{{ slspath + '/conf/jenkins_gracefull_restart.py' }}
        - mode: '0755'

/usr/local/bin/arcadia_pass_context.py:
    file.managed:
        - source: salt://components/docker-host/conf/arcadia_pass_context.py
        - mode: 755

/etc/default/jenkins:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/jenkins-defaults' }}
        - require:
            - pkg: jenkins
        - watch_in:
            - service: jenkins-service

jenkins-service:
    service.running:
        - name: jenkins
        - enable: true

/etc/nginx/ssl/jenkins.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/jenkins.pem:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/conf.d/jenkins.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/jenkins.conf' }}
        - require:
            - pkg: jenkins
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/logrotate.d/jenkins:
    file.managed:
        - source: salt://{{ slspath + '/conf/logrotate' }}
        - require:
            - pkg: jenkins

/etc/cron.d/wd-jenkins:
    file.managed:
        - source: salt://{{ slspath + '/conf/watchdog' }}
        - require:
            - service: jenkins

/var/backups/jenkins:
    file.directory:
        - user: jenkins
        - group: jenkins
        - dir_mode: 750
        - makedirs: True
        - require:
            - pkg: jenkins

jenkins-backup-pkgs:
    pkg.installed:
        - pkgs:
            - duplicity

/etc/cron.d/jenkins-backup:
    file.managed:
        - source: salt://{{ slspath + '/conf/jenkins-backup.cron' }}
        - template: jinja
        - require:
            - pkg: jenkins-backup-pkgs

/etc/logrotate.d/jenkins-backup:
    file.managed:
        - source: salt://{{ slspath + '/conf/jenkins-backup.logrotate' }}
        - require:
            - pkg: jenkins-backup-pkgs
