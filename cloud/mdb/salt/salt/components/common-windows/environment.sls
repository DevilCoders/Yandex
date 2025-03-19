set-system-path-environment-python:
    mdb_windows.add_to_system_path:
        - path: 'C:\salt\bin\'

set-system-path-environment-python-scripts:
    mdb_windows.add_to_system_path:
        - path: 'C:\salt\bin\Scripts'

set-system-path-environment-nssm:
    mdb_windows.add_to_system_path:
        - path: 'C:\Program Files\NSSM\tools\win64\'

set-system-path-environment-openssl:
    mdb_windows.add_to_system_path:
        - path: 'C:\Program Files\openssl\'
