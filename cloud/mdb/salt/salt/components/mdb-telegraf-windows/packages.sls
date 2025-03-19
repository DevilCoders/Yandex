iam-token-reissuer-package:
    mdb_windows.nupkg_installed:
       - name: IAMTokenReissuer
       - version: '1.7578662'
       - require:
           - cmd: mdb-repo

telegraf-package:
    mdb_windows.nupkg_installed:
       - name: Telegraf
       - version: '117.111147521'
       - stop_service: telegraf
       - require:
           - cmd: mdb-repo
