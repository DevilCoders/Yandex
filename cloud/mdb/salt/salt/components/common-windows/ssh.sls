authorized-keys-package:
    mdb_windows.nupkg_installed:
        - name: MdbSshKeys
        - version: '134.183331032'
        - require:
            - cmd: mdb-repo

authorized-keys-file:
    file.copy:
        - name: 'C:\ProgramData\ssh\administrators_authorized_keys'
        - source: 'C:\Program Files\MdbSshKeys\authorized_keys.root'
        - force: True
        - preserve: True
        - makedirs: True
        - onchanges:
            - mdb_windows: authorized-keys-package

fix-ssh-permissions:
    cmd.run:
        - shell: powershell
        - name: >
            Import-Module 'C:\Program Files\OpenSSH-Win64\OpenSSHUtils' -Force;
            Repair-SshdHostKeyPermission -FilePath 'C:\ProgramData\ssh\administrators_authorized_keys' -Confirm:$false
        - onchanges:
            - file: authorized-keys-file

sshd:
    service.running
