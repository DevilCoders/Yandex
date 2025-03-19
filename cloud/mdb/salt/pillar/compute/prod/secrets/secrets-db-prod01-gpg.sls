pubring.gpg: {{ salt.yav.get('ver-01eca5p0xcbqhmvmrn85tm365z[pubring.gpg]') | json }}
secring.gpg: {{ salt.yav.get('ver-01eca5p0xcbqhmvmrn85tm365z[secring.gpg]') | json }}
trustdb.gpg: {{ salt.yav.get('ver-01eca5p0xcbqhmvmrn85tm365z[trustdb.gpg]') | json }}
