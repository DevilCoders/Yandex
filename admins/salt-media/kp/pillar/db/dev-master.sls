{% set mgsec = "sec-01d74a0aqenzwhnbcg618jp4qc" %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}
