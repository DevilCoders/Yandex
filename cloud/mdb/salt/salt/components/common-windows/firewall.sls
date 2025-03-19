{% set nodes_ips = salt['mdb_windows.get_cluster_nodes_ip'](exclude_self = True) %}

windows-firewall-req:
    test.nop

windows-firewall-ready:
    test.nop

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}

mdb_metadata_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_metadata_tcp_out
    - protocol: tcp
    - remoteip: ["169.254.169.254"]
    - remoteport: 80
    - dir: out
    - enabled: true
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_cluster_tcp_in:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_cluster_tcp_in
    - localport: "135-140"
    - remoteport: any
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: in
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_cluster_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_cluster_tcp_out
    - localport: any
    - remoteport: "135-140"
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: out
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_445_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_445_tcp_out
    - localport: any
    - remoteport: 445
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: out
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_445_tcp_in:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_445_tcp_in
    - localport: 445
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - remoteport: any
    - protocol: tcp
    - dir: in
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_other_tcp_in:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_other_tcp_in
    - localport: "30000-65535"
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - remoteport: any
    - protocol: tcp
    - dir: in
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_other_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_other_tcp_out
    - localport: any
    - remoteport: "30000-65535"
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: out
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_fc_in:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_fc_in"
    - localport: 3343
    - remoteip: any
    - protocol: tcp
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_fc_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_fc_out"
    - localport: any
    - remoteport: 3343
    - remoteip: any
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_rpc_in:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_udp_135_in"
    - localport: 135
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_rpc_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_udp_135_out"
    - localport: any
    - remoteport: 135
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
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_udp_123_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_udp_123_out"
    - localport: any
    - remoteport: 123
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: udp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_fc_in_udp:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_fc_in_udp"
    - localport: 3343
    - protocol: udp
    - remoteip: any
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_fc_out_udp:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_fc_out_udp"
    - localport: any
    - remoteport: 3343
    - remoteip: any
    - protocol: udp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready
{% endif %}

allow_tcp80_internal_net:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_intranet_tcp80_out
    - localport: any
    - remoteport: 80
    - protocol: tcp
    - dir: out
    - action: allow
    - interface: eth1
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_tcp443_internal_net:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_intranet_tcp443_out
    - localport: any
    - remoteport: 443
    - protocol: tcp
    - dir: out
    - action: allow
    - interface: eth1
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_tcp4282_internal_net:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_intranet_tcp4282_out
    - localport: any
    - remoteport: 4282
    - protocol: tcp
    - dir: out
    - action: allow
    - interface: eth1
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_ssh_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_ssh_tcp_out
    - localport: any
    - remoteport: 22
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: out
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

mdb_intranet_tcp8443_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_intranet_tcp8443_out
    - remoteport: 8443
    - protocol: tcp
    - dir: out
    - action: allow
    - interface: eth1
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

open_ssh_tcp_in_22_port:
  mdb_windows.fw_netbound_rule_present:
    - name: "OpenSSH Server (sshd)"
    - localport: 22
    - protocol: tcp
    - dir: in
    - interface: eth1
    - action: allow
    - require:
      - test: windows-firewall-req
    - require_in:
      - test: windows-firewall-ready

open_ssh_tcp_in_22_port_nodesonly:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_usernet_tcp_22_in"
    - localport: 22
{% if not nodes_ips %}
    - remoteip: any
    - enabled: False
{%else%}
    - remoteip: {{ nodes_ips}}
{% endif%}
    - protocol: tcp
    - dir: in
    - action: allow
{% if not salt.pillar.get('data:mdb_plane') == 'control-plane' %}
    - interface: eth0
{% endif%}
    - require:
      - test: windows-firewall-req
    - require_in:
      - test: windows-firewall-ready

allow_icmp_v4:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_icmpv4_all_in
    - protocol: icmpv4
    - dir: in
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_icmp_v6:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_icmpv6_all_in
    - protocol: icmpv6
    - dir: in
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_icmp_v4_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_icmpv4_all_out
    - protocol: icmpv4
    - dir: out
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_icmp_v6_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_icmpv6_all_out
    - protocol: icmpv6
    - dir: out
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_dns_udp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dns_udp_out
    - localport: any
    - remoteport: 53
    - protocol: udp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_dns_tcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dns_tcp_out
    - localport: any
    - remoteport: 53
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_salt_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_salt_out
    - localport: any
    - remoteport: "4505-4506"
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_dhcp6_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dhcp6_out
    - localport: any
    - remoteport: 547
    - protocol: udp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_dhcp_out:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dhcp_out
    - localport: any
    - remoteport: 67
    - protocol: udp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_dhcp_in:
  mdb_windows.fw_netbound_rule_present:
    - name: mdb_dhcp_in
    - localport: 68
    - protocol: udp
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

disable_standard_rules:
  cmd.run:
    - shell: powershell
    - name: >
        get-netfirewallrule|where {$_.DisplayName -notlike 'MDB_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true'}|disable-netfirewallrule
    - unless: >
        if ((get-netfirewallrule|where {$_.DisplayName -notlike 'MDB_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true'}|measure-object|select count -ExpandProperty count) -eq 0) {exit 0} else {exit 1}
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready
