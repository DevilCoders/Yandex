'C:\Program Files\Mdb':
    file.directory:
        - user: Administrator

ps-profile:
    file.managed:
        - name: 'C:\Users\Administrator\Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1'
        - contents: |
            chcp 1250 > $null 
            function prompt { $ENV:COMPUTERNAME.ToLower() + " " + $(Get-Location).Path + " > "}

clear-nics-present:
    file.managed:
        - name: 'C:\Program Files\Mdb\clear_nics.ps1'
        - source: salt://{{ slspath }}/conf/clear_nics.ps1
        - require:
            - file: 'C:\Program Files\Mdb'
