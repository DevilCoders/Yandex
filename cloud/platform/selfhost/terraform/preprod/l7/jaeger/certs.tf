resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fd3a1u7vff9ptq092f37
  name = "jaeger"
  domains = [
    "jaeger.private-api.ycp.cloud-preprod.yandex.net",
    "jaeger-ydb.private-api.ycp.cloud-preprod.yandex.net",
    "jaeger-collector.private-api.ycp.cloud-preprod.yandex.net",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
