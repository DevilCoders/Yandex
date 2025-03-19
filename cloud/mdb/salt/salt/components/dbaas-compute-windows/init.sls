'C:\Program Files\Mdb\mount_data_disk.ps1':
    file.managed:
        - source: salt://{{ slspath }}/conf/mount_data_directory.ps1
        - require:
            - file: 'C:\Program Files\Mdb'

'C:\Program Files\Mdb\pre_restart.ps1':
    file.managed:
        - source: salt://{{ slspath }}/conf/pre_restart.ps1
        - require:
            - file: 'C:\Program Files\Mdb'

'C:\Program Files\Mdb\post_restart.ps1':
    file.managed:
        - source: salt://{{ slspath }}/conf/post_restart.ps1
        - require:
            - file: 'C:\Program Files\Mdb'

{% set disk_type_id = salt['pillar.get']('data:dbaas:disk_type_id') %}
disk-d-mounted:
    cmd.run:
        - shell: powershell
        - name: '& "C:\Program Files\Mdb\mount_data_disk.ps1" {{disk_type_id|yaml_encode}}'
        - require:
            - file: 'C:\Program Files\Mdb\mount_data_disk.ps1'
        - unless:
            - 'Get-Volume -DriveLetter D'

allow_wsus_tcp_8530_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_wsus_tcp_8530_out"
    - localport: any
    - remoteport: 8530
    - remoteip: any
    - interface: eth1
    - protocol: tcp
    - dir: out
    - action: allow

allow_wsus_tcp_8531_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_wsus_tcp_8531_out"
    - localport: any
    - remoteport: 8531
    - remoteip: any
    - interface: eth1
    - protocol: tcp
    - dir: out
    - action: allow

