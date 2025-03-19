salt_master:
  lookup:
    config: salt://master.conf
    csync2:
      from_pillar:
        csync2.key: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[alet.csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[alet.csync2.ssl_crt]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01czqm72888a4cfng0t7f4nfta[alet.csync2.ssl_key]') | json }}
    ssh:
      key: {{ salt.yav.get('sec-01czqjjqb24hsh0fy69t61vv5f[alet]') | json }}
    arcadia:
      remote_path: 'trunk/arcadia/admins/salt-media'
      local_target: '/srv/git'
      arc_token: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[arcanum_oauth_token]') | json }}
    monrun:
      memory_threshold: 75
