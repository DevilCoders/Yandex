comocutor:
  identity: {{ salt.yav.get('sec-01g4xkt7dt5wxh9y3r1t9a9zpk')['ssh_key'] | json }}
  identity_racktables: {{ salt.yav.get('sec-01g4xkt7dt5wxh9y3r1t9a9zpk')['ssh_key_rt'] | json }}
  rtpass: {{ salt.yav.get('sec-01g4xkt7dt5wxh9y3r1t9a9zpk')['rtpass'] | json }}
invapi:
  token: {{ salt.yav.get('sec-01g4xkt7dt5wxh9y3r1t9a9zpk')['invapi_token'] | json }}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g8vmvf14pv9htpbe4mbhr323')['client_secret'] | json}}
