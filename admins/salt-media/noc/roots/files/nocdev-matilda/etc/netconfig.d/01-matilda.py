def process(interfaces, params):
  for iface in interfaces:
    # Because yandex-udp-balancer forwards packets with "no defrag"
    iface.default_mtu = 8900
    iface.jumboroute = True

def filter(params):
  if 'jumboroute' in params.host_tags:
    return True
  return False
