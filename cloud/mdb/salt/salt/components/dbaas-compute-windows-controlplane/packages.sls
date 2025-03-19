selfdns-client-installed:
    mdb_windows.nupkg_installed:
       - name: selfdns-client
       - version: '56.11489795.0' 
       - require:
           - cmd: mdb-repo
