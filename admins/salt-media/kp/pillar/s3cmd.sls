s3cmd:
  s3_access_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessKeyId]")|json }}
  s3_secret_key: {{ salt.yav.get("sec-01fxqtbc2aga97btcrtmsf74aa[uploader_s3_AccessSecretKey]")|json }}
