gideon-pkg:
    pkg.installed:
        - pkgs:
              - yandex-gideon-mdb-bundle: '0.1.8680008'
        - require:
              - cmd: repositories-ready

/etc/gideon/secrets:
    file.managed:
        - contents_pillar: 'data:gideon:secret'
        - mode: 600
        - require:
              - pkg: gideon-pkg

# service file provided by yandex-gideon-systemd package (which is a yandex-gideon-mdb-bundle dependency)
gideon-service:
    service.running:
        - name: yandex-gideon
        - enable: True
        - watch:
              - pkg: gideon-pkg
              - file: /etc/gideon/secrets

