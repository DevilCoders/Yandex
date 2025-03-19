include:
  - .mdb-metrics

mdb-cms-pkgs:
  pkg.installed:
    - pkgs:
        - mdb-cms-instance-autoduty: '1.9783347'
    - require:
        - cmd: repositories-ready

mdb-cms-user:
  user.present:
    - fullname: MDB CMS system user
    - name: mdb-cms
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/etc/yandex/mdb-cms/mdb-cms-instance.yaml:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath + '/conf/mdb-cms-instance.yaml' }}
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

mdb-cms-instance-autoduty-service:
  service.running:
    - name: mdb-cms-instance-autoduty
    - enable: True
    - require:
        - pkg: mdb-cms-pkgs
        - user: mdb-cms
    - watch:
        - pkg: mdb-cms-pkgs
        - file: /etc/yandex/mdb-cms/mdb-cms-instance.yaml
        - file: /etc/systemd/system/mdb-cms-instance-autoduty.service

/etc/systemd/system/mdb-cms-instance-autoduty.service:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath + '/conf/mdb-cms-instance-autoduty.service' }}
    - require:
        - pkg: mdb-cms-pkgs
