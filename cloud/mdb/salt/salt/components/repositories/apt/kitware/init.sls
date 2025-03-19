{% set distrib = salt['grains.get']('oscodename') %}
kitware-repo:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb  https://apt.kitware.com/ubuntu/ {{ distrib }} main'
        - file: /etc/apt/sources.list.d/kitware.list
        - key_url: salt://components/repositories/apt/kitware/kitware.gpg
        - clean_file: true
