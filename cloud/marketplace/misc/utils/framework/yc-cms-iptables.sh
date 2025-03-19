generate_iptables() {
# IPv4
iptables -A INPUT -i lo -j ACCEPT # Allow loopback
iptables -A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A INPUT -p icmp -j ACCEPT # Allow icmp
iptables -A INPUT -p udp -m udp --dport 68 -j ACCEPT # DHCP client
iptables -A INPUT -p tcp --dport 22 -j ACCEPT # Allow ssh
iptables -A INPUT -p tcp --dport 80 -j ACCEPT # Allow http
iptables -A INPUT -p tcp --dport 443 -j ACCEPT # Allow https
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# IPv6
ip6tables -A INPUT -i lo -j ACCEPT # Allow loopback
ip6tables -A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT
ip6tables -A INPUT -p ipv6-icmp -j ACCEPT # Allow icmp6
ip6tables -A INPUT -p tcp --dport 22 -j ACCEPT # Allow ssh
ip6tables -A INPUT -p tcp --dport 80 -j ACCEPT # Allow http
ip6tables -A INPUT -p tcp --dport 443 -j ACCEPT # Allow https
ip6tables -P INPUT DROP
ip6tables -P FORWARD DROP
ip6tables -P OUTPUT ACCEPT

YC_WELCOME_BODY=`cat <<EOF
Only 80, 443 and 22 tcp ports are open by default
To view all network permissions exec “sudo iptables-save” and “sudo ip6tables-save”

${YC_WELCOME_BODY}
EOF`

}
