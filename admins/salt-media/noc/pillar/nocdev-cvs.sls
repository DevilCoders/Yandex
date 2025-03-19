certificates:
  contents  :
    tree.yandex.ru.key: {{ salt.yav.get('sec-01fekdsyw2hq2mxxp9t75gced3[7F001811C62CF4021FCE49691D0002001811C6_private_key]') | json }}
    tree.yandex.ru.pem: {{ salt.yav.get('sec-01fekdsyw2hq2mxxp9t75gced3[7F001811C62CF4021FCE49691D0002001811C6_certificate]') | json }}
  path      : "/etc/apache2/ssl/"
  packages  : ['apache2']
  services  : 'apache2'

sec: {{ salt.yav.get('sec-01fjes1qgfnptvn1h956e8yrz0') | json }}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g3wxaa9rpgxdzhts669f6gje')['client_secret']|json}}
