{% set salt_version = salt['pillar.get']('data:salt_version', '3000.9+ds-1+yandex0') %}

salt-api:
    service.running:
        - restart: True
        - require:
            - pkg: salt-packages
            - pkg: salt-api-packages
        - watch:
            - pkg: salt-dependency-packages
            - pkg: salt-packages
            - file: /etc/salt/master

salt-api-packages:
    pkg.installed:
        - pkgs:
            - salt-api: {{ salt_version }}
        - require:
            - pkg: salt-dependency-packages
            - cmd: repositories-ready

/etc/nginx/ssl/salt-api.pem:
    file.managed:
        - contents_pillar: cert.crt
        - makedirs: True
        - user: root
        - group: root
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/nginx/ssl/salt-api.key:
    file.managed:
        - contents_pillar: cert.key
        - makedirs: True
        - user: root
        - group: root
        - watch_in:
            - service: nginx-service
        - require_in:
            - service: nginx-service

/etc/logrotate.d/salt-api:
    file.managed:
       - source: salt://{{ slspath }}/conf/salt-api.logrotate
       - template: jinja
       - mode: 644
       - user: root
       - group: root

/etc/nginx/conf.d/salt-api.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/salt-api.nginx.conf
        - mode: 644
        - user: root
        - group: root
        - watch_in:
            - service: nginx-service

mdb-deploy-salt-api-user:
  user.present:
    - fullname: MDB Deploy salt-api system user
    - name: {{ salt['pillar.get']('data:mdb-deploy-salt-api:salt_api_user') }}
    - createhome: True
    - password: {{ salt['pillar.get']('data:mdb-deploy-salt-api:salt_api_password', '__NOPASSWORD__') }}
    - enforce_password: True
    - shell: /bin/false
    - system: True
    - groups:
        - www-data
