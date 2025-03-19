{% set version_string = salt.pillar.get("data:sqlserver:version:major_human") %}
{% set folder_name = salt['mdb_sqlserver.get_folder_by_version'](version_string) %}

sqlserver-config-req:
    test.nop

sqlserver-config-ready:
    test.nop

spconfigure_reg_ErrorLogSizeInKb_init:
    reg.present:
        - name: 'HKLM\SOFTWARE\Microsoft\Microsoft SQL Server\{{folder_name}}\MSSQLServer'
        - vname: 'ErrorLogSizeInKb'
        - vdata: 100000
        - vtype: REG_DWORD
        - require_in:
            - mdb_sqlserver_config: spconfigure_comply
            - test: sqlserver-config-ready
        - require:
            - test: sqlserver-config-req

spconfigure_reg_NumErrorLogs_init:
    reg.present:
        - name: 'HKLM\SOFTWARE\Microsoft\Microsoft SQL Server\{{folder_name}}\MSSQLServer'
        - vname: 'NumErrorLogs'
        - vdata: 10
        - vtype: REG_DWORD
        - require_in:
            - mdb_sqlserver_config: spconfigure_comply
            - test: sqlserver-config-ready
        - require:
            - test: sqlserver-config-req

{% set mem = salt.grains.get('mem_total') %}
{% if mem*0.15 < 2000 %}
{% set mem = mem - 2000 %}
{% else %}
{% set mem = mem - mem*0.15 %}
{% endif %}

spconfigure_comply:
    mdb_sqlserver_config.spconfigure_comply:
        - opts:
{% for k, v in salt['pillar.get']('data:sqlserver:config',{}).items() %}
            {{ k|yaml_encode }}: {{ v|yaml_encode }}
{% endfor %}
            "max_server_memory_(MB)": {{mem|round|int|yaml_encode}}
        - require:
            - test: sqlserver-config-req
        - require_in:
            - test: sqlserver-config-ready
