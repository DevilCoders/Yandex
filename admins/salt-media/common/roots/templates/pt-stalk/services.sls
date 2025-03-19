ubic-pt-stalk:
  service.running:
    - name: pt-stalk
    - enable: True
    - sig: 'bash /usr/bin/pt-stalk --config'
    - watch:
      - file: /etc/yandex/percona-toolkit/pt-stalk/config
      - file: /etc/yandex/percona-toolkit/pt-stalk/plugin
      - file: /etc/ubic/service/pt-stalk

