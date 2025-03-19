{% set mgsec = "sec-01d74a0cbwv4jvtc66jznmzq5s" %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}
