{% set yaenv = grains['yandex-environment'] %}
{% if yaenv == 'production' %}
s3cmd: {{ salt.yav.get("sec-01d5vaq16nvwnrs7swm1f2bqh6[s3cmd]")|json }}
{% else %}
s3cmd: {{ salt.yav.get("sec-01d5vaqn0px04g89wxty08mskp[s3cmd]")|json }}
{% endif %}
