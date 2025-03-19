certificates:
  source    : 'certs'
  contents  :
    l3mon.mds.yandex.net.key: {{ salt.yav.get('sec-01g3rgarzxg36qqj2ksrcqy5hp[7F001CE0E6CC5EF7EE054491690002001CE0E6_private_key]') | json }}
    l3mon.mds.yandex.net.pem: {{ salt.yav.get('sec-01g3rgarzxg36qqj2ksrcqy5hp[7F001CE0E6CC5EF7EE054491690002001CE0E6_certificate]') | json }}
    l3mon.mds.yandex-team.ru.key: {{ salt.yav.get('sec-01g3zr2k3kd9s8sxwd97y715vx[7F001CEE271805455D0FCAED4F0002001CEE27_private_key]') | json }}
    l3mon.mds.yandex-team.ru.pem: {{ salt.yav.get('sec-01g3zr2k3kd9s8sxwd97y715vx[7F001CEE271805455D0FCAED4F0002001CEE27_certificate]') | json }}

robot-natasha-solomon-oauth: {{ salt.yav.get('sec-01f2rfsg1m9t8xf81qahw79v7r[SOLOMON_OAUTH_TOKEN]') | json }}
