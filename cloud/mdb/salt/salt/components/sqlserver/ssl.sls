{% set version_string = salt.pillar.get("data:sqlserver:version:major_human") %}
{% set folder_name = salt['mdb_sqlserver.get_folder_by_version'](version_string) %}

external-cert-crt:
  file.managed:
      - name: 'C:\Program Files\Microsoft SQL Server\server.crt'
      - contents_pillar: cert.crt
      - user: SYSTEM
      - require_in:
          - test: sqlserver-service-req

external-cert-key:
  file.managed:
      - name: 'C:\Program Files\Microsoft SQL Server\server.key'
      - contents_pillar: cert.key
      - user: SYSTEM
      - require_in:
          - test: sqlserver-service-req


external-cert-pfx:
  cmd.run:
    - shell: powershell
    - name: >
        & 'C:\Program Files\openssl\openssl.exe' pkcs12 -keyex -export -passout pass:
        -out 'C:\Program Files\Microsoft SQL Server\server.pfx'
        -inkey 'C:\Program Files\Microsoft SQL Server\server.key'
        -in 'C:\Program Files\Microsoft SQL Server\server.crt'
    - require:
        - mdb_windows: openssl-package
    - onchanges:
        - file: external-cert-key
        - file: external-cert-crt


import-external-cert:
  cmd.run:
    - shell: powershell
    - name: >
        $crt = Import-PfxCertificate -CertStoreLocation cert:\LocalMachine\My -FilePath 'C:\Program Files\Microsoft SQL Server\server.pfx';
        $rsaCert = [System.Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPrivateKey($crt);
        $path = 'C:\ProgramData\Microsoft\Crypto\Keys\' + $rsaCert.key.UniqueName;
        $rule = New-Object System.Security.AccessControl.FileSystemAccessRule('NT Service\MSSQLSERVER', 'Read', 'None', 'None', 'Allow');
        Get-Acl -Path $path | %{ $_.AddAccessRule($rule); $_ } | Set-Acl -Path $path
    - onchanges:
        - cmd: external-cert-pfx
            

setup-sqlserver-external-cert:
  cmd.run:
    - shell: powershell
    - name: >
        $crt = Get-PfxCertificate -FilePath 'C:\Program Files\Microsoft SQL Server\server.pfx';
        Set-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Microsoft SQL Server\{{folder_name}}\MSSQLServer\SuperSocketNetLib' 
        -Name Certificate -Type String -Value $crt.Thumbprint
    - onchanges:
        - cmd: import-external-cert
    - require_in:
        - test: sqlserver-service-req


force-sqlserver-encryption:
  cmd.run:
    - shell: powershell
    - name: >
        Set-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Microsoft SQL Server\{{folder_name}}\MSSQLServer\SuperSocketNetLib'
        -Name 'ForceEncryption' -Type DWord -Value 1
    - unless: >
        $fe = Get-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Microsoft SQL Server\{{folder_name}}\MSSQLServer\SuperSocketNetLib' -Name 'ForceEncryption' | Select -Expand 'ForceEncryption';
        exit [int](1 -ne $fe)
    - require_in:
        - test: sqlserver-service-req
