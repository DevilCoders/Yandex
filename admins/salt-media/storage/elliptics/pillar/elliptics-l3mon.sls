certificates:
  source    : 'certs'
  contents  :
    l3mon.mds.yandex.net.key: {{ salt.yav.get('sec-01e9644ar8vkr2kftm2wjfvg0c[7F000D9D340CCA7511F61DE6280002000D9D34_private_key]') | json }}
    l3mon.mds.yandex.net.pem: {{ salt.yav.get('sec-01e9644ar8vkr2kftm2wjfvg0c[7F000D9D340CCA7511F61DE6280002000D9D34_certificate]') | json }}
    l3mon.mds.yandex-team.ru.key: {{ salt.yav.get('sec-01eaw2rdxxyr7t226gpe81aqdj[7F000E01B03ED971B426BD02E80002000E01B0_private_key]') | json }}
    l3mon.mds.yandex-team.ru.pem: {{ salt.yav.get('sec-01eaw2rdxxyr7t226gpe81aqdj[7F000E01B03ED971B426BD02E80002000E01B0_certificate]') | json }}

robot-overseer-solomon-oauth: {{ salt.yav.get('sec-01f2rfsg1m9t8xf81qahw79v7r[SOLOMON_OAUTH_TOKEN]') | json }}
