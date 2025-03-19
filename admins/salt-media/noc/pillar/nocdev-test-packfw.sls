cluster: nocdev-test-packfw

include:
    - units.ssl.nocdev-packfw

sec: {{salt.yav.get('sec-01efp5kt5wre3csb8aydd79sx6') | json}}
