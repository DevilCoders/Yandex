preinstall-salt-dependency-packages:
    pkg.installed:
        - pkgs:
            - python-requests
            - python-openssl
            - python-cherrypy3
            - python3-cherrypy3
            - python-gnupg
            - python-nacl
            - python3-nacl
            - python-paramiko
            - python3-paramiko
            - python-redis
            - python3-redis
            - python-git
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo
            - zk-flock
            - rsync
            - python-jwt
            - python3-jwt
            - python3-lxml
            - python-setproctitle
        - require:
            - cmd: repositories-ready

preinstall-some-packages:
    pkg.installed:
        - pkgs:
            - python3
            - python3-humanfriendly
            - python3-xmltodict
            - s3cmd
            - zk-flock
            - salt-master: 3002.7+ds-1+yandex0
            - salt-api: 3002.7+ds-1+yandex0
            - yandex-passport-tvmtool: 1.3.4
            - mdb-deploy-saltkeys: '1.8079300'
            - mdb-deploy-api: '1.8120995'
        - require:
            - pkg: preinstall-salt-dependency-packages
            - cmd: repositories-ready
