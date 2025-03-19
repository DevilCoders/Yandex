vsftpd:
    passwd: {{ salt.yav.get('sec-01d2d1j6t4kya79qfcyemr2rqk[passwd]') | json }}

amedia:
  cred: {{ salt.yav.get('sec-01dq7q6frhbs0ftjpyh1zh404f[item]') | json }}
