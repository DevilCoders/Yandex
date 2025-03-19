cluster: nocdev-test-4k

include:
  - units.ssl.nocdev-4k

sec: {{salt.yav.get('sec-01ee87ca4yqf9rzmw80zxrzw8q') | json}}

config:
  mongo:
    db_name: chk-testing

solomon:
  service: checkist-testing
  push_endpoint: /
  project: noc
