{% set distrib = salt['grains.get']('oscodename') %}
'docker-repo':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb [arch=amd64] https://download.docker.com/linux/ubuntu {{ distrib }} stable'
        - file: /etc/apt/sources.list.d/docker.list
        - require:
            - pkg: apt-transport-https
            - pkg: unbound-config-local64
            - file: docker-repo-gpg-key

docker-repo-gpg-key:
    file.managed:
        - name: /etc/apt/trusted.gpg.d/docker.gpg
        - source: salt://{{ slspath + '/conf/gpg.key' }}
        - onchanges_in:
            - cmd: repositories-ready

apt-transport-https:
    pkg.installed:
        - require_in:
            - cmd: repositories-ready
