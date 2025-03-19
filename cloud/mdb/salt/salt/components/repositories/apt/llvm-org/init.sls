{% set distrib = salt['grains.get']('oscodename') %}
llvm-org-repo:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb  http://apt.llvm.org/{{ distrib }} llvm-toolchain-{{ distrib }}-14 main'
        - file: /etc/apt/sources.list.d/llvm-org.list
        - key_url: salt://components/repositories/apt/llvm-org/llvm-org.gpg
        - clean_file: true
