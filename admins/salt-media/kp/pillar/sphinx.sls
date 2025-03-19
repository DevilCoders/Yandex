{%- if grains['yandex-environment'] in ["production"] %}
s3cmd:
  s3_access_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessKeyId]")|json }}
  s3_secret_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessSecretKey]")|json }}
kinopoisk_auth_pass: {{ salt.yav.get('sec-01d60t36bj4j03eb8z9khhtyd8[password]') | json }}
{% endif %}
