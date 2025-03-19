mysql_secrets: {{salt.yav.get('sec-01f2ph6x1k2ya1kkztzxmrcjaa')|json}}

mysync:
  cluster-id: racktables-testing
mysql:
  server-ids:
    iva-test-mysql01: 1
    sas-test-mysql01: 2
