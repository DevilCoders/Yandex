returner-scheduled:
    mdb_windows_tasks.present:
        - name: salt-returner
        - command: 'C:\salt\bin\python'
        - arguments: '"C:\Program Files\MdbConfigSalt\returners\mdb_salt_returner.py"'
        - schedule_type: 'Once'
        - repeat_interval: "1 minute"
        - location: salt
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True

nic-clear-scheduled:
    mdb_windows_tasks.present:
        - name: clear-nics
        - command: 'powershell.exe'
        - arguments: '-file "C:\Program Files\Mdb\clear_nics.ps1"'
        - schedule_type: 'OnBoot'
        - location: mdb
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True
        - require:
            - file: clear-nics-present
