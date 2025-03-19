{% from "components/repositories/apt/mdb-bionic/map.jinja" import mdb_bionic with context %}

'mdb-bionic-stable-all':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} stable/all/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-stable.list
        - onchanges_in:
            - cmd: repositories-ready
        - require:
            - file: /etc/apt/sources.list.d/mdb-bionic.list

'mdb-bionic-stable-arch':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ mdb_bionic.url }} stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/mdb-bionic-secure-stable.list
        - onchanges_in:
            - cmd: repositories-ready
        - require:
            - file: /etc/apt/sources.list.d/mdb-bionic.list

/etc/apt/sources.list.d/mdb-bionic-stable.list:
    file.absent:
        - require:
            - file: /etc/apt/sources.list.d/mdb-bionic.list

# Installed by dbaas-vm-images to that location.
/etc/apt/sources.list.d/mdb-bionic.list:
    file.absent
