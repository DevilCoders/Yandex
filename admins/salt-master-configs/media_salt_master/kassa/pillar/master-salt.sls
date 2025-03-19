salt_master:
  lookup:
    csync2:
      from_pillar:
        csync2.key: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[kassa.csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[kassa.csync2.ssl_crt]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[kassa.csync2.ssl_key]') | json }}
    ssh:
      key: {{ salt.yav.get('sec-01czqjjqb24hsh0fy69t61vv5f[kassa]') | json }}
    config: salt://master.conf
    git_local:
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
        - kassa:
            git: git@github.yandex-team.ru:salt-media/kassa.git
