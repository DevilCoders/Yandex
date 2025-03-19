nssm-package:
    mdb_windows.nupkg_installed:
       - name: NSSM
       - version: '2.24' 
       - require:
           - cmd: mdb-repo

openssl-package:
    mdb_windows.nupkg_installed:
       - name: openssl
       - version: '23691.14868637' 
       - require:
           - cmd: mdb-repo
