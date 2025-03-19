docker-repo-gpg-key:
    file.managed:
        - name: /etc/apt/trusted.gpg.d/docker.gpg
        - source: salt://{{ slspath + '/conf/docker.gpg' }}

docker-repo:
    pkgrepo.managed:
        - name: 'deb [arch=amd64] https://mirror.yandex.ru/mirrors/docker bionic stable'
        - file: /etc/apt/sources.list.d/docker.list
        - require:
            - file: docker-repo-gpg-key
        - required_in:
            - pkg: docker-pkgs

/root/.docker/config.json:
    file.managed:
        - source: salt://{{ slspath + '/conf/docker-config.json' }}
        - template: jinja
        - mode: '0600'
        - makedirs: True

docker-pkgs:
    pkg.installed:
        - pkgs:
            - docker-ce: '5:20.10.5~3-0~ubuntu-bionic'
        - require:
            - file: /root/.docker/config.json
