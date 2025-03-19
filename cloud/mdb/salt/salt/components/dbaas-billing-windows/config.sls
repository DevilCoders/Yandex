billing-config:
    file.managed:
        - name: 'C:\ProgramData\MdbBilling\billing.yaml'
        - source: salt://{{ slspath }}/conf/billing.yaml
        - template: jinja

billing-restart:
    mdb_windows.service_restarted:
        - service_name: 'MdbBilling'
        - onchanges:
            - file: billing-config

billing-firewall:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_2135_tcp_out
    - localport: any
    - remoteport: 2135
    - remoteip: any
    - protocol: tcp
    - dir: out
    - action: allow
    - interface: eth1
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready
