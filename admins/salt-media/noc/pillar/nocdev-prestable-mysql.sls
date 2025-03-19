mysql_secrets: {{salt.yav.get('sec-01f2t1ehz808tye807ea0n9rkk')|json}}
mysql_secrets_production: {{salt.yav.get('sec-01eyjnqk690dfvftz4vdxk06gg')|json}}

mysync:
  cluster-id: racktables-prestable
mysql:
  server-ids:
    vla-prestable-mysql01: 3
    sas-prestable-mysql01: 1
