pubring.gpg: {{ salt.yav.get('ver-01eca62ez7r95yd6s9nytwhy0n[pubring.gpg]') | json }}
secring.gpg: {{ salt.yav.get('ver-01eca62ez7r95yd6s9nytwhy0n[secring.gpg]') | json }}
trustdb.gpg: {{ salt.yav.get('ver-01eca62ez7r95yd6s9nytwhy0n[trustdb.gpg]') | json }}
