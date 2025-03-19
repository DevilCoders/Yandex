cluster: nocdev-4k

include:
  - units.ssl.nocdev-4k

sec: {{salt.yav.get('sec-01ee87ca4yqf9rzmw80zxrzw8q') | json}}
