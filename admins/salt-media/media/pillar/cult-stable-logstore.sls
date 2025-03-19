push_server:
  lookup:
    storage:
      filescount: 30

selfdns:
  token: {{ salt.yav.get('sec-01d2cny7z1jvg9syc695bbew2v[selfdns_oauth_token]') | json }}
