{% from "components/repositories/apt/mdb-bionic/map.jinja" import mdb_bionic with context %}

'mdb-bionic-testing-all':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} testing/all/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-testing.list
        - onchanges_in:
            - cmd: repositories-ready

'mdb-bionic-testing-arch':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} testing/$(ARCH)/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-testing.list
        - onchanges_in:
            - cmd: repositories-ready

