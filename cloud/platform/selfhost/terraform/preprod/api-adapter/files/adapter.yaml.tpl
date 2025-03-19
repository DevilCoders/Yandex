grpcServer:
  port: 8443
  threadPoolSize: 200
  queueSize: 20000

computeClient:
  protocol: https
  host: iaas.private-api.cloud-preprod.yandex.net
  port: 443
  connectTimeoutMs: 60000
  readTimeoutMs: 60000
  writeTimeoutMs: 60000

iamClient:
  protocol: https
  host: identity.private-api.cloud-preprod.yandex.net
  port: 14336
  connectTimeoutMs: 60000
  readTimeoutMs: 60000
  writeTimeoutMs: 60000
  disableSslVerification: true

mdbClient:
  protocol: https
  host: mdb.private-api.cloud-preprod.yandex.net
  port: 443
  connectTimeoutMs: 60000
  readTimeoutMs: 60000
  writeTimeoutMs: 60000
  disableSslVerification: true

metrics:
  port: 4440
