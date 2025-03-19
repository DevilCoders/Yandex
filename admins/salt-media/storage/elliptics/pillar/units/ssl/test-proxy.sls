certificates:
  source    : 'certs'

  contents  :
    s3-private.mdst.yandex.net.key: {{ salt.yav.get('sec-01efh6nm3ny30ea6fgpdm7hz5g[7F000F19BAB3C1DD4E5E31840E0002000F19BA_private_key]') | json }}
    s3-private.mdst.yandex.net.pem: {{ salt.yav.get('sec-01efh6nm3ny30ea6fgpdm7hz5g[7F000F19BAB3C1DD4E5E31840E0002000F19BA_certificate]') | json }}
    mds.yandex.net.key: {{ salt.yav.get('sec-01fedvvbgggp1e8t352yeb57rj[6DFF6E826EDEFEEBB2EBCE98569DABA8_private_key]') | json }}
    mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvvbgggp1e8t352yeb57rj[6DFF6E826EDEFEEBB2EBCE98569DABA8_certificate]') | json }}
    storage.stm.yandex.net.crt: {{ salt.yav.get('sec-01egk4pm9q7egv6bykg26wq17e[7F000F88B327DD03E3B130E6240002000F88B3_certificate]') | json }}
    storage.stm.yandex.net.pem: {{ salt.yav.get('sec-01egk4pm9q7egv6bykg26wq17e[7F000F88B327DD03E3B130E6240002000F88B3_private_key]') | json }}
    s3.mdst.yandex.net.key: {{ salt.yav.get('sec-01f0h4tw876ph0za232fed6gwt[7F0013C3598DE780DA95664E4300020013C359_private_key]') | json }}
    s3.mdst.yandex.net.pem: {{ salt.yav.get('sec-01f0h4tw876ph0za232fed6gwt[7F0013C3598DE780DA95664E4300020013C359_certificate]') | json }}
    s3-idm.mdst.yandex.net.key: {{ salt.yav.get('sec-01egk4pq59e3gjb8yvbytad359[7F000F88B405EA47CA282635E40002000F88B4_private_key]') | json }}
    s3-idm.mdst.yandex.net.pem: {{ salt.yav.get('sec-01egk4pq59e3gjb8yvbytad359[7F000F88B405EA47CA282635E40002000F88B4_certificate]') | json }}
    s3-website.mdst.yandex.net.key: {{ salt.yav.get('sec-01fbs9wrcsxjs5vb21c6f02fxv[7F00175097CAFC58444EEA257B000200175097_private_key]') | json }}
    s3-website.mdst.yandex.net.pem: {{ salt.yav.get('sec-01fbs9wrcsxjs5vb21c6f02fxv[7F00175097CAFC58444EEA257B000200175097_certificate]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
  cert_owner: s3proxy
  cert_group: s3proxy
