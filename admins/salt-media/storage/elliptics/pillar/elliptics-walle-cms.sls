include:
  - units.ssl.walle-cms

walle_conductor_token: {{ salt.yav.get('sec-01e5jgx318e14r9gnjnm7nhz5t[conductor_token]') | json }}
walle_ssh_key_pass: {{ salt.yav.get('sec-01e5jgx318e14r9gnjnm7nhz5t[ssh_password]')| json}}
walle_ssh_key: {{ salt.yav.get('sec-01e5jgx318e14r9gnjnm7nhz5t[ssh_key]')| json }}
walle_tvm_client_id: 2001243
