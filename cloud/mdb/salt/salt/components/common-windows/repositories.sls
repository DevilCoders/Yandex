mdb-repo:
    cmd.run:
        - shell: powershell
        - name: >
            Register-PackageSource 
            -Name MDB 
            -ProviderName NuGet 
            -Location https://mdb-windows.s3.mds.yandex.net/nupkg/index.json
            -Trusted 
            -WarningAction SilentlyContinue
        - unless: >
            Get-PackageSource -Name MDB
