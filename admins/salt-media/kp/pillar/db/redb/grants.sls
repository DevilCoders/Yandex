{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in ["production"] %}
{% set mgsec = "sec-01d72fw1h6snk8r9v61nf43y50" %}
{% elif yaenv in ["stress"] %}
{% set mgsec = "sec-01d72fwc8ttmv3f4tenq2e9nfn" %}
{% elif yaenv in ["testing"] %}
{% set mgsec = "sec-01d72fw7d3v0na5rwfr9zm59sc" %}
{% endif %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}
