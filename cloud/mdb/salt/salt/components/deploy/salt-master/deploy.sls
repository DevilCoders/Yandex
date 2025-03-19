mdb-deploy-saltkeys-user:
  user.present:
    - fullname: MDB Deploy SaltKeys system user
    - name: mdb-deploy-saltkeys
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

mdb-deploy-saltkeys-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-deploy-saltkeys: '1.9268650'
        - require:
            - cmd: repositories-ready

/etc/yandex/mdb-deploy-saltkeys/mdb-deploy-saltkeys.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-deploy-saltkeys.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-deploy-saltkeys-pkgs

mdb-deploy-saltkeys-service:
    service.running:
        - name: mdb-deploy-saltkeys
        - enable: True
        - watch:
            - pkg: mdb-deploy-saltkeys-pkgs
            - file: /etc/yandex/mdb-deploy-saltkeys/mdb-deploy-saltkeys.yaml
