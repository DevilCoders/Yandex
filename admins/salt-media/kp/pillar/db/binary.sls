s3cmd:
  s3_access_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessKeyId]")|json }}
  s3_secret_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessSecretKey]")|json }}

{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in ["production"] %}
{% set mgsec = "sec-01d72fw1h6snk8r9v61nf43y50" %}
{% endif %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}
