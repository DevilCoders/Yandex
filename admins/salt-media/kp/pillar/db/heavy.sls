{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in ["production"] %}
yt_token: {{ salt.yav.get("sec-01d5r9ym6fakn08car6hmqt8wh[item]") | json }}
{% endif %}
