envoy-pkgs:
    pkg.installed:
        - pkgs:
            - getenvoy-envoy: '1.18.3.p0.g98c1c9e-1p77.gb76c773+yandex0'
        - require:
            - cmd: repositories-ready

/etc/envoy:
    file.directory:
        - user: root
        - group: root
        - mode: 0750
        - makedirs: True

/etc/envoy/envoy.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/envoy.yaml' }}
        - mode: '0640'
        - user: root
        - group: root
        - require:
            - pkg: envoy-pkgs
            - /etc/envoy
        - require_in:
            - envoy-service
        - watch_in:
            - envoy-service

/etc/envoy/health_proto.pb:
    file.managed:
        - source: salt://{{ slspath + '/conf/health_proto.pb' }}
        - require:
            - pkg: envoy-pkgs
            - /etc/envoy
        - require_in:
            - envoy-service
        - watch_in:
            - envoy-service

/etc/envoy/ssl:
    file.directory:
        - user: root
        - group: root
        - mode: 0750
        - makedirs: True
        - require:
            - pkg: envoy-pkgs
            - file: /etc/envoy

/etc/envoy/ssl/service.key:
    file.managed:
        - mode: '0600'
        - contents_pillar: 'cert.key'
        - require:
            - /etc/envoy/ssl
        - watch_in:
            - service: envoy-service
        - require_in:
            - service: envoy-service

/etc/envoy/ssl/service.crt:
    file.managed:
        - mode: '0640'
        - contents_pillar: 'cert.crt'
        - require:
            - /etc/envoy/ssl
        - watch_in:
            - service: envoy-service
        - require_in:
            - service: envoy-service

/var/log/envoy:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - require:
            - pkg: envoy-pkgs
        - require_in:
            - envoy-service

/etc/systemd/system/envoy.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/envoy.service' }}
        - mode: '0640'
        - user: root
        - group: root
        - makedirs: True
        - require:
            - pkg: envoy-pkgs
        - require_in:
            - envoy-service
        - onchanges_in:
            - module: systemd-reload

/etc/logrotate.d/envoy:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

envoy-service:
    service.running:
        - name: envoy
        - enable: True
        - watch:
            - pkg: envoy-pkgs
