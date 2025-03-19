{% from "components/repositories/apt/mdb-bionic/map.jinja" import mdb_bionic with context %}

'mdb-bionic-unstable-all':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} unstable/all/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-unstable.list
        - onchanges_in:
            - cmd: repositories-ready

'mdb-bionic-unstable-arch':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} unstable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-unstable.list
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/sources.list.d/mdb-bionic-unstable.list:
    file.absent
