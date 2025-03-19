yav: {{ salt.yav.get('sec-01czwqr1y8j5pkbwqbteb6dsr9') | json }}
gpg_asc: {{ salt.yav.get('sec-01d2cny7z1jvg9syc695bbew2v[gpg_private_key]') | json }}
docker:
  version: 5:19.03.13~3-0~ubuntu-bionic
  legacy: False
