certificates:
  contents  :
    key.valve.yandex.net.pem: {{ salt.yav.get('sec-01fqxdk6g9k1f6wvf9pz5w8x0z[7F001A3C10AA72D1BF729713980002001A3C10_private_key]') | json }}
    cert.valve.yandex.net.pem: {{ salt.yav.get('sec-01fqxdk6g9k1f6wvf9pz5w8x0z[7F001A3C10AA72D1BF729713980002001A3C10_certificate]') | json }}
    key.valve.yandex-team.ru.pem: {{ salt.yav.get('sec-01fvm2et3s82fgs706f5bwjb7b[7F001B174C0A13EC655955CCAB0002001B174C_private_key]') | json }}
    cert.valve.yandex-team.ru.pem: {{ salt.yav.get('sec-01fvm2et3s82fgs706f5bwjb7b[7F001B174C0A13EC655955CCAB0002001B174C_certificate]') | json }}
  path      : "/etc/nginx/ssl"
  packages  : ['nginx'] 
  services  : 'nginx'

### NOCDEV-6124 experiment with salt-autodeploy
{% import_yaml slspath + '/autodeploy.yaml' as autodeploy_payload %}
salt-autodeploy: {{autodeploy_payload|json}}
salt-autodeploy-sls:
  units.percent:
  units.valve.autodeploy:
      monitor: True

unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g6a77zwfvbn59bywsqssywkb')['client_secret']|json}}
