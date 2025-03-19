certificates:
  source    : 'certs'

  contents  :
    avatars-int.mds.yandex.net.key: {{ salt.yav.get('sec-01fedvvdn2zs2re914jat7yqfa[74A6C40689E6765889D51B27752AE696_private_key]') | json }}
    avatars-int.mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvvdn2zs2re914jat7yqfa[74A6C40689E6765889D51B27752AE696_certificate]') | json }}
    avatars.mds.yandex.net.key: {{ salt.yav.get('sec-01fedvvfh1j94pf3n6sagjgja7[351F5450307CDEA0369E22F4E134E8D9_private_key]') | json }}
    avatars.mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvvfh1j94pf3n6sagjgja7[351F5450307CDEA0369E22F4E134E8D9_certificate]') | json }}

  path      : "/etc/yandex-certs/"
  packages  : ['nginx']
  service   : 'nginx'
  check_pkg : 'config-monrun-cert-check'
