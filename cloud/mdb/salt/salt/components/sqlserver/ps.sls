mdbsqlsrv-dir-present:
    file.directory:
        - name: 'C:\Program Files\WindowsPowerShell\Modules\mdbsqlsrv'
        - user: SYSTEM

mdbsqlsrv-module-present:
    file.managed:
        - name: 'C:\Program Files\WindowsPowerShell\Modules\mdbsqlsrv\mdbsqlsrv.psm1'
        - source: salt://{{ slspath }}/conf/mdbsqlsrv.psm1
        - require:
            - file: mdbsqlsrv-dir-present

vimrc-present:
    file.managed:
        - name: 'C:\Users\Administrator\.vimrc'
        - source: salt://{{ slspath }}/conf/vimrc
