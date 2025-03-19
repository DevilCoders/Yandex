#iptables -A INPUT -s 127.0.0.1/32 -j ACCEPT # Allow loopback
'allow loopback input':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: INPUT
    - source: 127.0.0.1
    - jump: ACCEPT
    - save: True

#iptables -A OUTPUT -s 127.0.0.1/32 -d 127.0.0.1/32 -j ACCEPT # Allow loopback
'allow loopback output':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: OUTPUT
    - source: 127.0.0.1
    - destination: 127.0.0.1
    - jump: ACCEPT
    - save: True


#iptables -A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT
'RELATED,ESTABLISHED':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: INPUT
    - match: state
    - connstate: RELATED,ESTABLISHED
    - jump: ACCEPT
    - save: True


#iptables -A INPUT -p icmp -j ACCEPT # Allow icmp
'allow icmp':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: INPUT
    - protocol: icmp
    - jump: ACCEPT
    - save: True

#iptables -A INPUT -p udp -m udp --dport 68 -j ACCEPT # DHCP client
'dhcp client':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: INPUT
    - protocol: udp
    - match: udp
    - dport: 68
    - jump: ACCEPT
    - save: True

#iptablse -A OUTPUT -p tcp -d 169.254.169.254 --dport 80 -j ACCEPT # Metadata service
'Metadata service':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: OUTPUT
    - protocol: tcp
    - destination: 169.254.169.254
    - dport: 80
    - jump: ACCEPT
    - save: True

#iptables -P INPUT DROP
'INPUT DROP':
  iptables.set_policy:
    - table: filter
    - family: ipv4
    - chain: INPUT
    - policy: DROP

#iptables -P FORWARD DROP
'FORWARD DROP':
  iptables.set_policy:
    - table: filter
    - family: ipv4
    - chain: FORWARD
    - policy: DROP

#iptables -P OUTPUT DROP
'OUTPUT DROP':
  iptables.set_policy:
    - table: filter
    - family: ipv4
    - chain: OUTPUT
    - policy: DROP
