yandex-returner-dir:
    file.directory:
        - name: 'C:\salt\var\run\mdb-salt-returner'
        - user: 'SYSTEM'
        - require:
            - file: yandex-dir

{% set deploy_version = salt['pillar.get']('data:deploy:version', 2) %}
{% set deploy_api_host = salt['pillar.get']('data:deploy:api_host', '') %}

deploy-version-config:
    file.managed:
        - name: 'C:\salt\conf\deploy_version'
        - replace: False
        - contents: |
            {{ deploy_version }}

{% if deploy_api_host != '' %}
deploy-api-host-config:
    file.managed:
        - name: 'C:\salt\conf\mdb_deploy_api_host'
        - contents: |
            {{ deploy_api_host }}
{% endif %}

deploy-job-result-blacklist-config:
    file.managed:
        - name: 'C:\salt\conf\job_result_blacklist.yaml'
        - source: salt://components/common/conf/etc/yandex/mdb-deploy/job_result_blacklist.yaml

config-salt-package:
    mdb_windows.nupkg_installed:
        - name: MdbConfigSalt
        - version: '1.8722387'
        - require:
            - cmd: mdb-repo

{# service install serction  #}

mdb_ping_salt_master_service_installed:
    mdb_windows.service_installed:
        - service_name: 'mdb_ping_salt_master'
        - service_call: 'C:\salt\bin\python.exe'
        - service_args: ['"C:\Program Files\MdbConfigSalt\mdb_ping_salt_master.py"']
        - require:
            - mdb_windows: nssm-package 
            - mdb_windows: set-system-path-environment-python
            - mdb_windows: set-system-path-environment-nssm

mdb_ping_salt_master_service_running:
    mdb_windows.service_running:
        - service_name: "mdb_ping_salt_master"
        - require:
            - mdb_windows: mdb_ping_salt_master_service_installed
