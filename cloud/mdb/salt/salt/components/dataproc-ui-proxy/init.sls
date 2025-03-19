dataproc-ui-proxy-pkgs:
    pkg.installed:
        - pkgs:
            - dataproc-ui-proxy: '1.9585154'
        - require:
            - cmd: repositories-ready

dataproc-ui-proxy-user:
    user.present:
        - fullname: Data Proc UI Proxy user
        - name: dataproc-ui-proxy
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

/etc/yandex/dataproc-ui-proxy:
    file.directory:
        - user: dataproc-ui-proxy
        - mode: 755
        - makedirs: True
        - require:
            - user: dataproc-ui-proxy-user

/etc/yandex/dataproc-ui-proxy/dataproc-ui-proxy.yaml:
    file.serialize:
        - dataset_pillar: data:dataproc-ui-proxy:config
        - formatter: yaml
        - mode: 640
        - user: dataproc-ui-proxy
        - makedirs: True
        - require:
            - user: dataproc-ui-proxy-user
            - file: /etc/yandex/dataproc-ui-proxy

/var/log/dataproc-ui-proxy:
    file.directory:
        - user: root
        - group: root
        - makedirs: True

/etc/logrotate.d/dataproc-ui-proxy:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

dataproc-ui-proxy-service:
    service.running:
        - name: dataproc-ui-proxy
        - enable: True
        - require:
              - file: /var/log/dataproc-ui-proxy
              - user: dataproc-ui-proxy-user
        - watch:
              - pkg: dataproc-ui-proxy-pkgs
              - file: /etc/yandex/dataproc-ui-proxy/dataproc-ui-proxy.yaml
