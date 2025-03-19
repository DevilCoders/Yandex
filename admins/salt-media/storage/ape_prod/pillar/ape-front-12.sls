include:
  - units.push-client.front-12
  - units.ssl-ape-front-12

tls_session_tickets:
  /etc/nginx/ssl/:
    ape.key: {{ salt.yav.get('sec-01dzv1q1f54x4hczq6tx1svh01[0]') | json }}
    ape.key.prev: {{ salt.yav.get('sec-01dzv1q1f54x4hczq6tx1svh01[1]') | json }}
    ape.key.prevprev: {{ salt.yav.get('sec-01dzv1q1f54x4hczq6tx1svh01[2]') | json }}

juggler_user: root
juggler_daemon_user: root
juggler_hack_porto: True

walle_enabled: True

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700|vlan788"
