{% set osrelease = salt['grains.get']('osrelease') %}
include:
    - .mdb-metrics
    - components.logrotate

mdb-secrets-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-secrets: '1.9268650'

mdb-secrets-user:
  user.present:
    - fullname: MDB Secrets system user
    - name: mdb-secrets
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/opt/yandex/mdb-secrets/mdb-secrets.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-secrets.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-secrets-pkgs

/etc/nginx/ssl/mdb-secrets.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

{% set crt_path = '/etc/nginx/ssl/mdb-secrets.pem' %}

{{ crt_path }}:
    file.managed:
        - mode: '0644'
        - contents_pillar: 'cert.crt'
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

{% from "components/monrun2/http-tls/filecheck.sls" import tls_check_certificate with context %}
{{ tls_check_certificate(crt_path) }}

/etc/nginx/conf.d/mdb-secrets.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/nginx-api.conf' }}
        - require:
            - pkg: mdb-secrets-pkgs
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/lib/systemd/system/mdb-secrets.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-secrets.service
        - mode: 644
        - require:
            - pkg: mdb-secrets-pkgs
            - user: mdb-secrets-user
        - onchanges_in:
            - module: systemd-reload


/etc/yandex/mdb-secrets:
    file.directory:
        - user: mdb-secrets
        - group: www-data
        - mode: '0750'
        - makedirs: True
        - require:
            - user: mdb-secrets-user

/var/log/mdb-secrets:
    file.directory:
        - user: mdb-secrets
        - group: www-data
        - mode: 755
        - recurse:
            - user
            - group
            - mode
        - makedirs: True
        - require:
            - user: mdb-secrets-user

/etc/logrotate.d/mdb-secrets:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

mdb-secrets-service:
    service.running:
        - name: mdb-secrets
        - enable: True
        - watch:
            - pkg: mdb-secrets-pkgs
            - file: /lib/systemd/system/mdb-secrets.service
            - file: /opt/yandex/mdb-secrets/mdb-secrets.yaml
            - file: /var/log/mdb-secrets
