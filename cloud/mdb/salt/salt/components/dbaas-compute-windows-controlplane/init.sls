include:
    - .packages
    - .selfdns-client-plugin
    - .selfdns-client

mdb-dir-present:
    file.directory:
        - name: 'C:\ProgramData\yandex\mdb'
        - user: SYSTEM

flags-dir-present:
    file.directory:
        - name: 'C:\ProgramData\yandex\mdb\flags'
        - user: SYSTEM
        - require: 
            - file: mdb-dir-present
