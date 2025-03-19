{% set unit = "racktables" %}

sec_vm-rt-test: {{salt.yav.get('sec-01e57g77d1q5bjwxybd3xh7d3q') | json}}
sec_rt-yandex: {{salt.yav.get('sec-01ex7wbyyw047qm8gmb819cpea') | json}}
sec_rt-yandex-tokens: {{salt.yav.get('sec-01ex7xkvph0m96zfgngtsw13tz') | json}}
sec_crt: {{salt.yav.get('sec-01emrj6vzhngqpfd4msd3w57jb') | json}}

unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01e86r8bnpxhektcwg7jv7ghbe')['client_secret']|json}}
