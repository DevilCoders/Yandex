certificates:
  contents  :
    cvs-test.net.yandex.net.key: {{ salt.yav.get('sec-01fgs1cx1fyh4ecxgrvkx66kme[7F0018A29243B8B628E447EBBD00020018A292_private_key]') | json }}
    cvs-test.net.yandex.net.pem: {{ salt.yav.get('sec-01fgs1cx1fyh4ecxgrvkx66kme[7F0018A29243B8B628E447EBBD00020018A292_certificate]') | json }}
  path      : "/etc/apache2/ssl/"
  packages  : ['apache2']
  services  : 'apache2'

sec: {{ salt.yav.get('sec-01fjes1qgfnptvn1h956e8yrz0') | json }}
unified_agent:
  tvm-client-secret: hey i'm secret!)
