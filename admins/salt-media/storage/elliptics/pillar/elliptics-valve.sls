iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
certificates:
  contents  :
    key.valve.yandex.net.pem: {{ salt.yav.get('sec-01dy53nj2n9kxh5wb4k8swxwcq[7F000B1C23AD601B204E0DBB090002000B1C23_private_key]') | json }}
    cert.valve.yandex.net.pem: {{ salt.yav.get('sec-01dy53nj2n9kxh5wb4k8swxwcq[7F000B1C23AD601B204E0DBB090002000B1C23_certificate]') | json }}
    key.valve.yandex-team.ru.pem: {{ salt.yav.get('sec-01e2jjnjj51cssxpyggdbr8ez8[7F000C1627CCA39C733EF392D80002000C1627_private_key]') | json }}
    cert.valve.yandex-team.ru.pem: {{ salt.yav.get('sec-01e2jjnjj51cssxpyggdbr8ez8[7F000C1627CCA39C733EF392D80002000C1627_certificate]') | json }}
  path      : "/etc/nginx/ssl/"
  packages  : ['nginx'] 
  services  : 'nginx'

