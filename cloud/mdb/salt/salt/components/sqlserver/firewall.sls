{% set nodes_ips = salt['mdb_windows.get_cluster_nodes_ip'](exclude_self = True) %}

sqlserver-firewall-req:
    test.nop:
      - require:
        - test: windows-firewall-ready

sqlserver-firewall-ready:
    test.nop

open_sql_tcp_in_1433_port:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_tsql_1433
    - localport: 1433
    - protocol: tcp
    - dir: in
{% if not salt['pillar.get']('data:running_in_dataproc_tests', False) %}
    - interface: eth0
{% endif %}
    - action: allow
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready

{% if salt['pillar.get']('data:access:web_sql_nets') %}
open_sql_tcp_in_1433_port_web_sql:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_tsql_1433_web_sql
    - localport: 1433
    - remoteip: {{ salt['pillar.get']('data:access:web_sql_nets') }}
{% if salt['pillar.get']('data:access:web_sql', False) %}
    - action: allow
{% else %}
    - action: block
{% endif %}
    - protocol: tcp
    - dir: in
    - interface: eth1
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready
{% endif %}

{% if salt['pillar.get']('data:access:data_lens_nets') %}
open_sql_tcp_in_1433_port_data_lens:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_tsql_1433_data_lens
    - localport: 1433
    - remoteip: {{ salt['pillar.get']('data:access:data_lens_nets') }}
{% if salt['pillar.get']('data:access:data_lens', False) %}
    - action: allow
{% else %}
    - action: block
{% endif %}
    - protocol: tcp
    - dir: in
    - interface: eth1
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready
{% endif %}

{% if salt['pillar.get']('data:access:data_transfer_nets') %}
open_sql_tcp_in_1433_port_data_transfer:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_tsql_1433_data_transfer
    - localport: 1433
    - remoteip: {{ salt['pillar.get']('data:access:data_transfer_nets') }}
{% if salt['pillar.get']('data:access:data_transfer', False) %}
    - action: allow
{% else %}
    - action: block
{% endif %}
    - protocol: tcp
    - dir: in
    - interface: eth1
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready
{% endif %}

open_sql_tcp_out_1433_port_usernet:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_tsql_1433_out_usernet
    - remoteport: 1433
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready  

open_sql_tcp_in_1434_port:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dac_1434
    - localport: 1434
    - protocol: tcp
    - dir: in
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
    - interface: eth1
{% endif %}
    - action: allow
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready

open_sql_tcp_in_5022_port:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_5022_tcp_in
    - localport: 5022
    - protocol: tcp
    - dir: in
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
    - interface: eth0
{% endif %}
    - action: allow
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready
open_sql_tcp_out_5022_port:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_5022_tcp_out
    - localport: any
    - remoteport: 5022
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
    - interface: eth0
{% endif %}
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: sqlserver-firewall-req
    - require_in:
        - test: sqlserver-firewall-ready
