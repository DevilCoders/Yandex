certificates:
  source    : 'certs'

  modes     :
    allCAs.pem: "0444"

  contents  :
    allCAs.pem: {{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[allCAs.pem]') | json }}
    s3.yandex.net.key: {{ salt.yav.get('sec-01fedwptcpv3atg9d5pn8hh5rk[3DC85E36FAEECF701038F51FA7C13050_private_key]') | json }}
    s3.yandex.net.pem: {{ salt.yav.get('sec-01fedwptcpv3atg9d5pn8hh5rk[3DC85E36FAEECF701038F51FA7C13050_certificate]') | json }}
    s3-private.mds.yandex.net.key: {{ salt.yav.get('sec-01fedvjj10p6bqhc5twh5dnzn7[3156037874F098CB796A03B781320D6E_private_key]') | json }}
    s3-private.mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvjj10p6bqhc5twh5dnzn7[3156037874F098CB796A03B781320D6E_certificate]') | json }}
    s3.mds.yandex.net.key: {{ salt.yav.get('sec-01fedvjnfex0ssxyvsap1na6b5[6CA4DB551ED21C5CA61AD41AE0172560_private_key]') | json }}
    s3.mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvjnfex0ssxyvsap1na6b5[6CA4DB551ED21C5CA61AD41AE0172560_certificate]') | json }}
    s3-idm.mds.yandex.net.key: {{ salt.yav.get('sec-01fedvjkw1zxqcxqrt7j13aa4v[356EB65C46C3A6360798BAA611AE8C70_private_key]') | json }}
    s3-idm.mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvjkw1zxqcxqrt7j13aa4v[356EB65C46C3A6360798BAA611AE8C70_certificate]') | json }}
    s3-website.mds.yandex.net.key: {{ salt.yav.get('sec-01fj9emppax1mx9ajb91jtpsbz[5D1676B65EA08F3F224539B0AC899769_private_key]') | json }}
    s3-website.mds.yandex.net.pem: {{ salt.yav.get('sec-01fj9emppax1mx9ajb91jtpsbz[5D1676B65EA08F3F224539B0AC899769_certificate]') | json }}

    storage.yandex-team.ru.key: {{ salt.yav.get('sec-01fedvjg4em3d8bbb8hrqt44tm[18A4B2A76DDDB5A56D865664F95AFC46_private_key]') | json }}
    storage.yandex-team.ru.pem: {{ salt.yav.get('sec-01fedvjg4em3d8bbb8hrqt44tm[18A4B2A76DDDB5A56D865664F95AFC46_certificate]') | json }}
    storage.mail.yandex.net.crt: {{ salt.yav.get('sec-01egg1pwvnkhhfhjp2b5kr7p3y[7F000F544BC6FDED02B4BDBB230002000F544B_certificate]') | json }}
    storage.mail.yandex.net.pem: {{ salt.yav.get('sec-01egg1pwvnkhhfhjp2b5kr7p3y[7F000F544BC6FDED02B4BDBB230002000F544B_private_key]') | json }}

    mds.yandex.net.key: {{ salt.yav.get('sec-01fedvje4a5vtt85yh9c4vs4x7[50E37B723B3004AC6E204D1A6DF1B6B1_private_key]') | json }}
    mds.yandex.net.pem: {{ salt.yav.get('sec-01fedvje4a5vtt85yh9c4vs4x7[50E37B723B3004AC6E204D1A6DF1B6B1_certificate]') | json }}

  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
  cert_owner: s3proxy
  cert_group: s3proxy
