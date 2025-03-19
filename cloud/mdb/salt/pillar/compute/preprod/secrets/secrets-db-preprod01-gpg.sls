pubring.gpg: {{ salt.yav.get('ver-01eca5b3da530arhz20xvj4btp[pubring.gpg]') | json }}
secring.gpg: {{ salt.yav.get('ver-01eca5b3da530arhz20xvj4btp[secring.gpg]') | json }}
trustdb.gpg: {{ salt.yav.get('ver-01eca5b3da530arhz20xvj4btp[trustdb.gpg]') | json }}
