cluster: nocdev-ck

include:
  - units.ssl.nocdev-ck

sec: {{salt.yav.get('sec-01ee87ca4yqf9rzmw80zxrzw8q') | json}}
