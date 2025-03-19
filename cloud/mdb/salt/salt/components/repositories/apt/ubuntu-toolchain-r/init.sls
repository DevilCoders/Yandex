{% set distrib = salt['grains.get']('oscodename') %}
ubuntu-toolchain-r-repo:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu {{ distrib }} main'
        - file: /etc/apt/sources.list.d/ubuntu-toolchain-r.list
        - require:
            - file: ubuntu-toolchain-r-repo-gpg-key

ubuntu-toolchain-r-repo-gpg-key:
    file.managed:
        - name: /etc/apt/trusted.gpg.d/ubuntu-toolchain-r.gpg
        - source: salt://{{ slspath + '/conf/gpg.key' }}
        - onchanges_in:
            - cmd: repositories-ready
