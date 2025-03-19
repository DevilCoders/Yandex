{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in ["production"] %}
{% set mgsec = "sec-01d72fw1h6snk8r9v61nf43y50" %}
{% endif %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}
