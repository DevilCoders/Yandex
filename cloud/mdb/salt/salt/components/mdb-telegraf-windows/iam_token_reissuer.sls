iam-token-reissuer-config-dir:
    file.directory:
        - name: 'C:\ProgramData\IAMTokenReissuer'
        - user: 'SYSTEM'

iam-token-reissuer-config:
    file.managed:
        - name: 'C:\ProgramData\IAMTokenReissuer\IAMTokenReissuer.yaml'
        - source: salt://{{ slspath }}/conf/IAMTokenReissuer.yaml
        - template: jinja
        - require:
            - file: iam-token-reissuer-config-dir

iam-token-reissuer-sa-private-key:
    file.managed:
        - name: 'C:\ProgramData\IAMTokenReissuer\sa-private-key.key'
        - contents_pillar: data:solomon_cloud:sa_private_key
        - require:
            - file: iam-token-reissuer-config-dir

iam-token-require-jwt-token-file:
    file.managed:
        - name: 'C:\ProgramData\IAMTokenReissuer\iam_token.txt'
        - require:
            - file: iam-token-reissuer-config-dir

iam-scheduled-task-extra-run:
    cmd.run:
        - shell: powershell
        - name: >
            &'C:\Program Files\IAMTokenReissuer\iam_token_reissuer.exe'
            --config=C:\ProgramData\IAMTokenReissuer\IAMTokenReissuer.yaml
        - onchanges:
            - file: iam-token-reissuer-config
        - require:
            - mdb_windows_tasks: iam-reissuer-scheduled

iam-reissuer-scheduled:
    mdb_windows_tasks.present:
        - name: iam-token-reissuer
        - command: 'C:\Program Files\IAMTokenReissuer\iam_token_reissuer.exe'
        - arguments: '--config=C:\ProgramData\IAMTokenReissuer\IAMTokenReissuer.yaml'
        - schedule_type: 'Once'
        - repeat_interval: "30 minutes"
        - location: salt
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True
        - require:
            - mdb_windows: iam-token-reissuer-package 
